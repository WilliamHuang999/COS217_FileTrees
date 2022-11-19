/*--------------------------------------------------------------------*/
/* nodef.c                                                            */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nodef.h"

/* A file node in a FT */
struct nodeF {
   /* Object corresponding to the node's absolute path */
   Path_T oPPath;

   /* Size of file contents in bytes */
   size_t ulLength;

   /* Contents of the file */
   void *pvContents;
};

/* ================================================================== */
int NodeF_new(Path_T oPPath, NodeF_T *poNFResult) {
   NodeF_T oNFNew;   /* New file node to be created */
   Path_T oPNewPath; /* New path of new file node */
   int iStatus;

   assert(oPPath != NULL);

   /* Path of file cannot be 0 or 1 */
   if (!(Path_getDepth(oPPath) > 1)) {
      *pONFResult = NULL;
      return NO_SUCH_PATH;
   }

   /* Allocate mem for new node and check for enough mem */
   oNFNew = (oNFNew)malloc(sizeof(struct nodeF));
   if(oNFNew == NULL) {
      *poNFResult = NULL;
      return MEMORY_ERROR;
   }

   /* Set the new node's path and check for enough mem */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(oNFNew);
      *poNFResult = NULL;
      return iStatus;
   }
   oNFNew->oPPath = oPNewPath;

   /* Set initial values of file contents and size*/
   oNFNew->ulLength = 0;
   oNFNew->pvContents = NULL;

   *poNFResult = oNFNew;

   return SUCCESS;
}

/* ================================================================== */
size_t NodeF_free(NodeF_T oNFNode) {
   size_t ulSize;

   assert(oNFNode != NULL);

   ulSize = oNFNode->ulLength;

   /* Remove path */
   Path_free(oNFNode->oPPath);
   /* Free the actual file node */
   free(oNFNode);

   return ulSize;
}

/* ================================================================== */
Path_T NodeF_getPath(NodeF_T oNFNode) {
   assert(oNFNode != NULL);
   return oNFNode->oPPath;
}

/* ================================================================== */
int NodeF_compare(NodeF_T oNFNode1, NodeF_T oNFNode2) {
   assert(oNFNode1 != NULL);
   assert(oNFNode2 != NULL);

   return Path_comparePath(oNFNode1->oPPath, oNFNode2->oPPath);
}

/* ================================================================== */
char *NodeF_toString(NodeF_T oNFNode) {
   char *pcResult;   /* String representation of oNFNode */

   assert(oNFNode != NULL);

   /* Allocate mem for pcResult and check if enough mem */
   pcResult = malloc(Path_getStrLength(Node_getPath(oNFNode))+1);
   if(pcResult == NULL) {
      return NULL;
   }
   /* Copy path name to pcResult and return the copy */
   return strcpy(pcResult, Path_getPathname(Node_getPath(oNFNode)));
}