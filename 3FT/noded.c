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

    /* the object containing links to this node's children that are 
    files */
    DynArray_T oDFileChildren;

    /* the object containg links to this node's children that are 
    directories */
    DynArray_T oDDirChildren;
};

/* ================================================================== */
/*
  Links new directory child oNdChild into oNdParent's directory 
  children array at index ulIndex. Returns SUCCESS if the new directory 
  child was added successfully, or  MEMORY_ERROR if allocation fails 
  adding oNdChild to the directory children array.
*/
static int NodeD_addDirChild(NodeD_T oNdParent, NodeD_T oNdChild,
                        size_t ulIndex) {
    assert(oNdParent != NULL);
    assert(oNdChild != NULL);

   /* insert into directory children array at user-given index */
    if(DynArray_addAt(oNdParent->oDDirChildren, ulIndex, oNdChild))
        return SUCCESS;
    else
        return MEMORY_ERROR;
}

/* Removes and frees all file children from oNdNode. */
static void NodeD_removeFileChildren(NodeD_T oNdNode){
   size_t numFileChildren;
   size_t numFileChildren2;

   assert(oNdNode != NULL);

   /* Get number of file children initially, copy used for safety 
   checks */
   numFileChildren = NodeD_getNumFileChildren(oNdNode);
   numFileChildren2 = NodeD_getNumFileChildren(oNdNode);
   /* Loop thru array of file children and remove/free them */
   while(numFileChildren != 0) {
      /* free memory and remove from dynarray */
      NodeF_free(DynArray_get(oNdNode->oDFileChildren,0));
      (void)DynArray_removeAt(oNdNode->oDFileChildren,0);

      /* check that a file child was actually removed*/
      numFileChildren2 -= 1;
      numFileChildren = NodeD_getNumFileChildren(oNdNode);
      assert(numFileChildren == numFileChildren2);
   }
   /* Free array of file children */
   DynArray_free(oNdNode->oDFileChildren);  
}


/*
  Compares the string representation of a directory node oNdNode1 with 
  a string pcSecond representing a directory node's path.
  Returns <0, 0, or >0 if oNdNode1 is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int NodeD_compareString(const NodeD_T oNdNode1,
                                 const char *pcSecond) {
   assert(oNdNode1 != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNdNode1->oPPath, pcSecond);
}

/* ================================================================== */
int NodeD_new(Path_T oPPath, NodeD_T oNdParent, NodeD_T *poNdResult) {
   struct nodeD *psdNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);
   assert(poNdResult != NULL);

   ulIndex = 0;

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
      /* depth of closest directory ancestor, should be difference of 1 
      (checked later) */
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
   /* parent of root is NULL */
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

   return SUCCESS;
}

/* ================================================================== */
int NodeD_addFileChild(NodeD_T oNdParent, NodeF_T oNfChild, size_t 
ulIndex) {
   assert(oNdParent != NULL);
   assert(oNfChild != NULL);

   if (DynArray_addAt(oNdParent->oDFileChildren, ulIndex, oNfChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/* ================================================================== */
size_t NodeD_free(NodeD_T oNdNode) {
   size_t ulIndex;
   size_t ulCount;

   assert(oNdNode != NULL);

   /* remove from parent's list */
   if(oNdNode->oNdParent != NULL) {
      /* Search for directory in parent's directory children array and 
      sets index in the array */
      if(DynArray_bsearch(
            oNdNode->oNdParent->oDDirChildren,
            oNdNode, &ulIndex,
            (int (*)(const void *, const void *)) NodeD_compare)) {
               /* Remove in the parent's directory children array at 
               the index found */
               (void) DynArray_removeAt
               (oNdNode->oNdParent->oDDirChildren,
                                  ulIndex);
            }
   }

   /* Recursively remove directory children */
   while(DynArray_getLength(oNdNode->oDDirChildren) != 0) {
      /* Increment counter of directories removed */
      ulCount += NodeD_free(DynArray_get(oNdNode->oDDirChildren, 0));
   }
   /* free the node's children */
   DynArray_free(oNdNode->oDDirChildren);

   /* Removes and frees file children (hence no free after) */
   NodeD_removeFileChildren(oNdNode);

   /* remove path */
   Path_free(oNdNode->oPPath);

   /* finally, free the struct node */
   free(oNdNode);
   ulCount++;
   return ulCount;
}

/* ================================================================== */
Path_T NodeD_getPath(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oPPath;
}

/* ================================================================== */
boolean NodeD_hasDirChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNdParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* returns results of binary search of directory child array, 
   *pulChildID is the index into oNdParent->oDDirChildren, gets set by 
   DynArray_bsearch */
   return (DynArray_bsearch(oNdParent->oDDirChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeD_compareString));
}

/* ================================================================== */
boolean NodeD_hasFileChild(NodeD_T oNdParent, Path_T oPPath,
                         size_t *pulChildID) {
   assert(oNdParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* returns results of binary search of file child array, *pulChildID 
   is the index into oNdParent->oDFileChildren, gets set by 
   DynArray_bsearch */
   return (DynArray_bsearch(oNdParent->oDFileChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) NodeF_compareString));
}

/* ================================================================== */
size_t NodeD_getNumDirChildren(NodeD_T oNdParent) {
   assert(oNdParent != NULL);

   /* length of file child array */
   return DynArray_getLength(oNdParent->oDDirChildren);
}

/* ================================================================== */
size_t NodeD_getNumFileChildren(NodeD_T oNdParent) {
   assert(oNdParent != NULL);

   /* length of file child array */
   return DynArray_getLength(oNdParent->oDFileChildren);
}

/* ================================================================== */
int  NodeD_getDirChild(NodeD_T oNdParent, size_t ulChildID,
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

/* ================================================================== */
int  NodeD_getFileChild(NodeD_T oNdParent, size_t ulChildID,
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

/* ================================================================== */
NodeD_T NodeD_getParent(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oNdParent;
}

/* ================================================================== */
int NodeD_compare(NodeD_T oNdNode1, NodeD_T oNdNode2) {
   assert(oNdNode1 != NULL);
   assert(oNdNode2 != NULL);

   return Path_comparePath(oNdNode1->oPPath, oNdNode2->oPPath);
}

/* ================================================================== */
char *NodeD_toString(NodeD_T oNdNode) {
   char *pcResult;  /* Resulting string representation to be returned */
   size_t totalStrlen; /* Total string length of pcResult */
   size_t i;   /* Index to iterate thru file children */
   size_t numFileChildren; /* Number of file children of directory */
   NodeF_T oNfChild; /* File child of oNdNode*/

   assert(oNdNode != NULL);

   totalStrlen = Path_getStrLength(NodeD_getPath(oNdNode)) + 1;

   /* Find out how many characters will be in pcResult */
   numFileChildren = NodeD_getNumFileChildren(oNdNode);
   for (i = 0; i < numFileChildren; i++) {
      oNfChild = DynArray_get(oNdNode->oDFileChildren,i);
      totalStrlen += Path_getStrLength(NodeF_getPath(oNfChild)) + 1;
   }

   /* Allocate mem and check if enough mem */
   pcResult = malloc(totalStrlen + 1);
   if (pcResult == NULL) {
      return NULL;
   }
   
   *pcResult = '\0';

   /* Concatenate oNdNode directory path name onto pcResult */
   strcat(pcResult, Path_getPathname(NodeD_getPath(oNdNode)));
   strcat(pcResult,"\n");

   /* Concatenate child file path names onto pcResult */
   for (i = 0; i < numFileChildren; i++) {
      oNfChild = DynArray_get(oNdNode->oDFileChildren,i);
      strcat(pcResult, Path_getPathname(NodeF_getPath(oNfChild)));
      strcat(pcResult,"\n");
   }

   return pcResult;
}

/* Returns the dynarray object representing the children of oNdNode 
that are files */
DynArray_T NodeD_getFileChildren(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oDFileChildren;
}

/* Returns the dynarray object representing the children of oNdNode 
that are directories */
DynArray_T NodeD_getDirChildren(NodeD_T oNdNode) {
   assert(oNdNode != NULL);

   return oNdNode->oDDirChildren;
}