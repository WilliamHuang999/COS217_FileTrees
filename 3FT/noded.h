/*--------------------------------------------------------------------*/
/* noded.h                                                            */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#ifndef NODED_INCLUDED
#define NODED_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A NodeD_T is a node in a Directory Tree */
typedef struct nodeD *NodeD_T;

/*
  Creates a new directory node in Directory Tree, with path oPPath and
  parent oNDParent. Returns an int SUCCESS status and sets *poNResult
  to be the new node if successful. Otherwise, sets *poNResult to NULL
  and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNDParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNDParent's path is not oPPath's direct parent
                 or oNDParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNDParent already has a child with this path
*/
int NodeD_new(Path_T oPPath, NodeD_T oNDParent, NodeD_T *poNResult);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNDNode, i.e., deletes this directory and all its descendents. 
  Returns the number of directories and files deleted.
*/
size_t NodeD_free(NodeD_T oNDNode);

/* Returns the path object representing oNDNode's absolute path. */
Path_T NodeD_getPath(NodeD_T oNDNode);

/*
  Returns TRUE if oNDParent has a child dir with path oPPath. Returns
  FALSE if it does not.

  If oNDParent has such a child, stores in *pulChildID the child's
  identifier (as used in NodeD_getChild). If oNDParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean NodeD_hasDChild(NodeD_T oNDParent, Path_T oPPath,
                         size_t *pulChildID);

/* Returns the number of children that oNDParent has. */
size_t NodeD_getNumDChildren(NodeD_T oNDParent);

/*
  Returns an int SUCCESS status and sets *poNResult to be the child
  node of oNDParent with identifier ulChildID, if one exists.
  Otherwise, sets *poNResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNDParent
*/
int NodeD_getDChild(NodeD_T oNDParent, size_t ulChildID,
                  NodeD_T *poNResult);

/*
  Returns a the parent node of oNDNode.
  Returns NULL if oNDNode is the root and thus has no parent.
*/
NodeD_T NodeD_getParent(NodeD_T oNDNode);

/*
  Compares oNDNode1 and oNDNode2 lexicographically based on their paths.
  Returns <0, 0, or >0 if oNDNode1 is "less than", "equal to", or
  "greater than" oNDNode2, respectively.
*/
int NodeD_compare(NodeD_T oNDNode1, NodeD_T oNDNode2);

/*
  Returns a string representation for oNDNode, or NULL if
  there is an allocation error.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *NodeD_toString(NodeD_T oNDNode);

#endif
