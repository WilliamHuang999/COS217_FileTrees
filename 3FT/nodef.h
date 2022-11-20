/*--------------------------------------------------------------------*/
/* nodef.h                                                            */
/* Author: Will Huang and George Tziampazis                           */
/*--------------------------------------------------------------------*/

#ifndef NODEF_INCLUDED
#define NODEF_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A NodeF_T is a node in a Directory Tree */
typedef struct nodeF *NodeF_T;

/*
  Creates a new file node in File Tree, with path oPPath. Returns an int SUCCESS status and sets *poNFResult to be the new node
  if successful. Otherwise, sets *poNFResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * NO_SUCH_PATH if oPPath is of depth 0 or 1 (root cannot be a file)
*/
int NodeF_new(Path_T oPPath, NodeF_T *poNfResult);

/*
  Destroys and frees all memory allocated to this file node
  oNFNode, i.e. Returns number of bytes the file contents had.
*/
size_t NodeF_free(NodeF_T oNfNode);

/* Returns the path object representing oNFNode's absolute path. */
Path_T NodeF_getPath(NodeF_T oNfNode);

/*
  Compares oNFNode1 and oNFNode2 lexicographically based on their paths.
  Returns <0, 0, or >0 if oNFNode1 is "less than", "equal to", or
  "greater than" oNFNode2, respectively.
*/
int NodeF_compare(NodeF_T oNfNode1, NodeF_T oNfNode2);

/*
  Compares the string representation of a file node oNfNode1 with a string pcSecond representing a file node's path.
  Returns <0, 0, or >0 if oNdNode1 is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
int NodeF_compareString(const NodeF_T oNfNode1, const char *pcSecond);

/*
  Returns a string representation for oNFNode, or NULL if
  there is an allocation error.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *NodeF_toString(NodeF_T oNfNode);

#endif
