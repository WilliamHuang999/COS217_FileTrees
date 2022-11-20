/*--------------------------------------------------------------------*/
/* noded.c                                                            */
/* Author: George Tziampazis, Will Huang                              */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "noded.h"
#include "nodef.h"

/* A directory node in a DT */
struct nodeD {
    /* the object corresponding to the node's absolute path */
    Path_T oPPath;
    /* this node's parent */
    NodeD_T oNParent;
    /* the object containing links to this node's children that are files */
    DynArray_T oDFileChildren;
    /* the object containg links to this node's children that are directories */
    DynArray_T oDDirChildren;
};


/*
  Links new file child oNChild into oNParent's file children array at index ulIndex. Returns SUCCESS if the new file child was added successfully, or  MEMORY_ERROR if allocation fails adding oNChild to the file children array.
*/
static int NodeD_addFileChild(NodeD_T oNParent, NodeF_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);

   if(DynArray_addAt(oNParent->oDFileChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Links new directory child oNChild into oNParent's directory children array at index ulIndex. Returns SUCCESS if the new directory child was added successfully, or  MEMORY_ERROR if allocation fails adding oNChild to the directory children array.
*/
static int NodeD_addDirChild(NodeD_T oNParent, NodeD_T oNChild,
                        size_t ulIndex) {
    assert(oNParent != NULL);
    assert(oNChild != NULL);

    if(DynArray_addAt(oNParent->odDirChildren, ulIndex, oNChild))
        return SUCCESS;
    else
        return MEMORY_ERROR;
}

/*
  Compares the string representation of a directory node oNfirst with a string pcSecond representing a directory node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int NodeD_compareString(const NodeD_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}

static int NodeF_compareString(const NodeF_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}

/*
  Creates a new directory node with path oPPath and directory parent oNParent.  Returns an int SUCCESS status and sets *poNResult to be the new directory node if successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int NodeD_new(Path_T oPPath, NodeD_T oNParent, NodeD_T *poNResult) {
   struct nodeD *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);
   assert(oNParent == NULL);

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct nodeD));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(NodeD_hasChild(oNParent, oPPath, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
   }
   psNew->oNParent = oNParent;

   /* initialize the new node */
   psNew->oDFileChildren = DynArray_new(0);
   psNew->oDDirChildren = DynArray_new(0);
   if(psNew->oDFileChildren == NULL || psNew->oDDirChildren == NULL) {
      Path_free(psNew->oPPath);
      free(psNew);
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = NodeD_addDirChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }

   *poNResult = psNew;

   assert(oNParent == NULL);

   return SUCCESS;
}

size_t NodeD_free(NodeD_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);

   /* remove from parent's list */
   if(oNNode->oNParent != NULL) {
      if(DynArray_bsearch(
            oNNode->oNParent->oDDirChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
         (void) DynArray_removeAt(oNNode->oNParent->oDDirChildren,
                                  ulIndex);
   }

   /* recursively remove directory children */
   while(DynArray_getLength(oNNode->oDDirChildren) != 0) {
      ulCount += NodeD_free(DynArray_get(oNNode->oDDirChildren, 0));
   }
   DynArray_free(oNNode->oDDirChildren);

   /* recursively remove file children */
   while(DynArray_getLength(oNNode->oDFileChildren) != 0) {
      ulCount += NodeF_free(DynArray_get(oNNode->oDFileChildren, 0));
   }
   DynArray_free(oNNode->oDFileChildren);

   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode);
   ulCount++;
   return ulCount;
}

Path_T NodeD_getPath(NodeD_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

/* MAKE SURE SETTING THE IDS ISNT FUCKED UP */
boolean NodeD_hasChild(NodeD_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* *pulChildID is the index into oNParent->oDChildren */
   return (DynArray_bsearch(oNParent->oDDirChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeD_compareString) || DynArray_bsearch(oNParent->oDFileChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeF_compareString));
}

size_t NodeD_getNumDirChildren(NodeD_T oNParent) {
   assert(oNParent != NULL);

   return DynArray_getLength(oNParent->oDDirChildren);
}

size_t NodeD_getNumFileChildren(NodeD_T oNParent) {
   assert(oNParent != NULL);

   return DynArray_getLength(oNParent->oDFileChildren);
}

int  NodeD_getDirChild(NodeD_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= NodeD_getNumDirChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
    /* Check where it exists (which array) then store in poNResult */
      *poNResult = DynArray_get(oNParent->oDDirChildren, ulChildID);
      return SUCCESS;
   }
}

int  NodeD_getFileChildren(NodeD_T oNParent, size_t ulChildID,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= NodeD_getNumFileChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
    /* Check where it exists (which array) then store in poNResult */
      *poNResult = DynArray_get(oNParent->oDFileChildren, ulChildID);
      return SUCCESS;
   }
}

Node_T NodeD_getParent(NodeD_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int NodeD_compare(NodeD_T oNFirst, NodeD_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

/* compare dir to file, dir always will come before file in definition, make clear in .h file */
int NodeD_compare(NodeD_T oNFirst, NodeF_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *NodeD_toString(NodeD_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(NodeD_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(NodeD_getPath(oNNode)));
}
