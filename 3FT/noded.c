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
    NodeD_T oNdParent;

    /* the object containing links to this node's children that are files */
    DynArray_T oDFileChildren;
    
    /* the object containg links to this node's children that are directories */
    DynArray_T oDDirChildren;
};


/*
  Links new file child oNfChild into oNdParent's file children array at index ulIndex. Returns SUCCESS if the new file child was added successfully, or  MEMORY_ERROR if allocation fails adding oNfChild to the file children array.
*/
static int NodeD_addFileChild(NodeD_T oNdParent, NodeF_T oNfChild,
size_t ulIndex) {
   assert(oNdParent != NULL);
   assert(oNfChild != NULL);

   if(DynArray_addAt(oNdParent->oDFileChildren, ulIndex, oNfChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Links new directory child oNdChild into oNdParent's directory children array at index ulIndex. Returns SUCCESS if the new directory child was added successfully, or  MEMORY_ERROR if allocation fails adding oNdChild to the directory children array.
*/
static int NodeD_addDirChild(NodeD_T oNdParent, NodeD_T oNdChild,
                        size_t ulIndex) {
    assert(oNdParent != NULL);
    assert(oNdChild != NULL);

    if(DynArray_addAt(oNdParent->oDDirChildren, ulIndex, oNdChild))
        return SUCCESS;
    else
        return MEMORY_ERROR;
}

/*
  Compares the string representation of a directory node oNdNode1 with a string pcSecond representing a directory node's path.
  Returns <0, 0, or >0 if oNdNode1 is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int NodeD_compareString(const NodeD_T oNdNode1,
                                 const char *pcSecond) {
   assert(oNdNode1 != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNdNode1->oPPath, pcSecond);
}

/*
  Creates a new directory node with path oPPath and directory parent oNdParent.  Returns an int SUCCESS status and sets *poNdResult to be the new directory node if successful. Otherwise, sets *poNdResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNdParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNdParent's path is not oPPath's direct parent
                 or oNdParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNdParent already has a child with this path
*/
int NodeD_new(Path_T oPPath, NodeD_T oNdParent, NodeD_T *poNdResult) {
   struct nodeD *psdNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);
   assert(oNdParent == NULL);

   /* allocate space for a new node */
   psdNew = malloc(sizeof(struct nodeD));
   if(psdNew == NULL) {
      *poNdResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psdNew);
      *poNdResult = NULL;
      return iStatus;
   }
   psdNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNdParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNdParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psdNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psdNew->oPPath);
         free(psdNew);
         *poNdResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psdNew->oPPath) != ulParentDepth + 1) {
         Path_free(psdNew->oPPath);
         free(psdNew);
         *poNdResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(NodeD_hasDirChild(oNdParent, oPPath, &ulIndex)) {
         Path_free(psdNew->oPPath);
         free(psdNew);
         *poNdResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psdNew->oPPath) != 1) {
         Path_free(psdNew->oPPath);
         free(psdNew);
         *poNdResult = NULL;
         return NO_SUCH_PATH;
      }
   }
   psdNew->oNdParent = oNdParent;

   /* initialize the new node */
   psdNew->oDFileChildren = DynArray_new(0);
   psdNew->oDDirChildren = DynArray_new(0);
   if(psdNew->oDFileChildren == NULL || psdNew->oDDirChildren == NULL) {
      Path_free(psdNew->oPPath);
      free(psdNew);
      *poNdResult = NULL;
      return MEMORY_ERROR;
   }

   /* Link into parent's children list */
   if(oNdParent != NULL) {
      iStatus = NodeD_addDirChild(oNdParent, psdNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psdNew->oPPath);
         free(psdNew);
         *poNdResult = NULL;
         return iStatus;
      }
   }

   *poNdResult = psdNew;

   assert(oNdParent == NULL);

   return SUCCESS;
}

size_t NodeD_free(NodeD_T oNdNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNdNode != NULL);

   /* remove from parent's list */
   if(oNdNode->oNdParent != NULL) {
      if(DynArray_bsearch(
            oNdNode->oNdParent->oDDirChildren,
            oNdNode, &ulIndex,
            (int (*)(const void *, const void *)) NodeD_compare)
        )
         (void) DynArray_removeAt(oNdNode->oNdParent->oDDirChildren,
                                  ulIndex);
   }

   /* recursively remove directory children */
   while(DynArray_getLength(oNdNode->oDDirChildren) != 0) {
      ulCount += NodeD_free(DynArray_get(oNdNode->oDDirChildren, 0));
   }
   DynArray_free(oNdNode->oDDirChildren);

   /* recursively remove file children */
   while(DynArray_getLength(oNdNode->oDFileChildren) != 0) {
      ulCount += NodeF_free(DynArray_get(oNdNode->oDFileChildren, 0));
   }
   DynArray_free(oNdNode->oDFileChildren);

   /* remove path */
   Path_free(oNdNode->oPPath);

   /* finally, free the struct node */
   free(oNdNode);
   ulCount++;
   return ulCount;
}

Path_T NodeD_getPath(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oPPath;
}

boolean NodeD_hasDirChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNdParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* *pulChildID is the index into oNdParent->oDDirChildren */
   return (DynArray_bsearch(oNdParent->oDDirChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeD_compareString));
}

boolean NodeD_hasFileChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNdParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* *pulChildID is the index into oNdParent->oDDirChildren */
   return (DynArray_bsearch(oNdParent->oDFileChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeF_compareString));
}

size_t NodeD_getNumDirChildren(NodeD_T oNdParent) {
   assert(oNdParent != NULL);

   return DynArray_getLength(oNdParent->oDDirChildren);
}

size_t NodeD_getNumFileChildren(NodeD_T oNdParent) {
   assert(oNdParent != NULL);

   return DynArray_getLength(oNdParent->oDFileChildren);
}

int NodeD_getDirChild(NodeD_T oNdParent, size_t ulChildID,
                   NodeD_T *poNdResult) {

   assert(oNdParent != NULL);
   assert(poNdResult != NULL);

   /* ulChildID is the index into oNdParent->oDDirChildren */
   if(ulChildID >= NodeD_getNumDirChildren(oNdParent)) {
      *poNdResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
    /* Check where it exists (which array) then store in poNdResult */
      *poNdResult = DynArray_get(oNdParent->oDDirChildren, ulChildID);
      return SUCCESS;
   }
}

int NodeD_getFileChild(NodeD_T oNdParent, size_t ulChildID,
                   NodeF_T *poNfResult) {

   assert(oNdParent != NULL);
   assert(poNfResult != NULL);

   /* ulChildID is the index into oNdParent->oDFileChildren */
   if(ulChildID >= NodeD_getNumFileChildren(oNdParent)) {
      *poNfResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
    /* Check where it exists (which array) then store in poNfResult */
      *poNfResult = DynArray_get(oNdParent->oDFileChildren, ulChildID);
      return SUCCESS;
   }
}

NodeD_T NodeD_getParent(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oNdParent;
}

int NodeD_compare(NodeD_T oNdNode1, NodeD_T oNdNode2) {
   assert(oNdNode1 != NULL);
   assert(oNdNode2 != NULL);

   return Path_comparePath(oNdNode1->oPPath, oNdNode2->oPPath);
}

/* compare dir to file, dir always will come before file in definition, make clear in .h file */
/*int NodeD_compare(NodeD_T oNdNode1, NodeF_T oNfNode2) {
   assert(oNdNode1 != NULL);
   assert(oNfNode2 != NULL);

   return Path_comparePath(oNdNode1->oPPath, oNfNode2->oPPath);
}*/

char *NodeD_toString(NodeD_T oNdNode) {
   char *copyPath;

   assert(oNdNode != NULL);

   copyPath = malloc(Path_getStrLength(NodeD_getPath(oNdNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(NodeD_getPath(oNdNode)));
}
