/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;  /* Parent of oNNode */
   Node_t oNChild1;
   Node_t oNChild2;
   Path_T oPNPath;   /* Path of oNNode */
   Path_T oPPPath;   /* Path of parent node */
   Path_T oPChildPath1;
   Path_T oPChildPath2;
   size_t i;
   size_t j;
   int iStatus;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   /* I THINK THIS INVARIANT WILL SOLVE DTBAD2 */
   /* Invariant: two nodes cannot have the same absolute path */
   /* int Path_comparePath(Path_T oPPath1, Path_T oPPath2); */

   oPNPath = Node_getPath(oNNode);
   for(i = 0; i < Node_getNumChildren(oNNode)-1; i++)
   {
      oNChild1 = NULL;
      iStatus = Node_getChild(oNNode, i, &oNChild1);

      if(iStatus != SUCCESS) {
         fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
         return FALSE;
      }

      oPChildPath1 = Node_getPath(oNChild1);
      if (Path_comparePath(oPNPath, oPChildPath1) == 0) {
         fprintf(stderr, "Two nodes have same absolute path.");
         return FALSE;
      }
      for (j = i; j < Node_getNumChildren(oNNode); j++) {

         oNChild2 = NULL;
         iStatus = Node_getChild(oNNode,j,&oNChild2);

         if (iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }
         oPChildPath2 = Node_getPath(oNChild2);
         if (Path_comparePath(oPChildPath1,oPChildPath2) == 0) {
            fprintf(stderr, "Two nodes have same absolute path.");
            return FALSE;
         }
      }

   /* I THINK THIS INVARIANT WILL SOLVE DTBAD3 */
   /* Invariant: children should be in lexicographic order */
   /* check if children of this node are in lexicographic order */   

   return TRUE;
   }
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;
   /*Path_T oPChildPath;
   Path_T oPNodePath;*/

   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr,"getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }

   /* Invariant: if DT not initialized, root node should be NULL. */
   if(!bIsInitialized) {
      if (oNRoot != NULL) {
         fprintf(stderr, 
         "Not initialized, but root node is not NULL\n");
         return FALSE;
      }
   }

   /* Invariant: if oNRoot is NULL, DT count should be 0. */
   if (oNRoot == NULL) {
      if (ulCount != 0) {
         fprintf(stderr, "Root node is NULL, but count is not 0\n");
         return FALSE;
      }
   }

   /* Invariant: if oNRoot is not NULL, DT count should not be 0. */
   if (oNRoot != NULL) {
      if (ulCount == 0) {
         fprintf(stderr, "Root node is not NULL, but count is 0\n");
         return FALSE;
      }
   }

   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
}
