/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: George Tziampazis, Will Huang                              */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "noded.h"
#include "nodef.h"
#include "ft.h"

/*
  A File Tree is a representation of a hierarchy of directories and
  files: the File Tree is rooted at a directory, directories
  may be internal nodes or leaves, and files are always leaves. It is 
  represented as an AO with 3 state variables:
*/

/* Variables to keep track of FT characteristics: */
/* 1. Flag for being in initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. Pointer to root directory node in the FT */
static NodeD_T oNRoot;
/* 3. Counter of number of directories (not including files) in FT */
static size_t ulDirCount;

/* ================================================================== */
/*
  Traverses the FT starting at the root to the farthest possible 
  DIRECTORY following absolute path oPPath. If able to traverse, 
  returns an int SUCCESS status and sets *poNFurthest to the furthest 
  directory node reached (which may be only a prefix of oPPath, or even 
  NULL if the root is NULL). Otherwise, sets *poNFurthest to NULL and 
  returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request

  *Credit: Adapted from DT_traversePath() (Christopher Moretti)
*/
static int FT_traversePath(Path_T oPPath, NodeD_T *poNFurthest) {
    int iStatus;
    Path_T oPPrefix = NULL;
    NodeD_T oNCurr;
    NodeD_T oNChild;
    size_t ulDepth, ulChildID;
    size_t i;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if(oNRoot == NULL) {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    /* checking depth of oPPath is valid */
    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        *poNFurthest = NULL;
        return iStatus;
    }

    if(Path_comparePath(NodeD_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    /* Increment over depths until at closest ancestor directory of 
    last node in the path. If the last node is a directory, it will 
    stop there. */
    for (i = 2; i <= ulDepth; i++) {
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }
        if (NodeD_hasDirChild(oNCurr, oPPrefix, &ulChildID)) {
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = NodeD_getDirChild(oNCurr, ulChildID, &oNChild);
            if (iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
        }
        else {
            break;
        }
    }
    Path_free(oPPrefix);
    *poNFurthest = oNCurr;
    return SUCCESS;
}

/* ================================================================== */
static int FT_findDir(const char *pcPath, NodeD_T *poNResult) {
    Path_T oPPath = NULL;
    NodeD_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    iStatus = FT_traversePath(oPPath, &oNFound);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if(oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    if(Path_comparePath(NodeD_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NOT_A_DIRECTORY;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}

/* ================================================================== */
static int FT_findFile(const char *pcPath, NodeF_T *poNResult) {
    int iStatus;
    Path_T oPPath = NULL;
    Path_T oPParentPath = NULL;
    NodeD_T oNParent = NULL;
    NodeF_T oNFound = NULL;
    size_t ulChildID;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    iStatus = FT_traversePath(oPPath, &oNParent);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if(oNParent == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    iStatus = Path_prefix(oPPath,Path_getDepth(oPPath)-1,&oPParentPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    if(Path_comparePath(NodeD_getPath(oNParent), oPParentPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }


    if (!NodeD_hasFileChild(oNParent, oPPath, &ulChildID)) {
        Path_free(oPPath);
        return NO_SUCH_PATH;
    }
    iStatus = NodeD_getFileChild(oNParent, ulChildID, &oNFound);
    if (iStatus != SUCCESS) {
        return iStatus;
    }   

    Path_free(oPPath);
    Path_free(oPParentPath);

    *poNResult = oNFound;
    return SUCCESS;
}

/* ================================================================== */
/*
   Inserts a new directory into the FT with absolute path pcPath.
   Returns SUCCESS if the new directory is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertDir(const char *pcPath) {
    int iStatus;
    Path_T oPPath = NULL;
    NodeD_T oNFirstNew = NULL;
    NodeD_T oNCurr = NULL;
    size_t ulDepth, ulIndex;
    size_t ulNewNodes = 0; 

    if(!bIsInitialized)
        return INITIALIZATION_ERROR;
    
    /* validate pcPath and generate a Path_T for it */
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS)
        return iStatus;
    
    /* find the closest directory ancestor of oPPath already in the 
    tree, ancestor must be a directory by definition of file tree */
    iStatus= FT_traversePath(oPPath, &oNCurr);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
    if(oNCurr == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if(oNCurr == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeD_getPath(oNCurr)) + 1;
        /* oNCurr is the node we're trying to insert */
        if (ulIndex == ulDepth + 1 && !Path_comparePath(oPPath, 
        NodeD_getPath(oNCurr))) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNCurr, build rest of the path one level at a time */
    while (ulIndex <= ulDepth) {
        Path_T oPPrefix = NULL;
        NodeD_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }

        /* If trying to insert child of a file */
        if (FT_containsFile(Path_getPathname(oPPrefix))) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL){
                (void)NodeD_free(oNFirstNew);
            }
            return NOT_A_DIRECTORY;
        }

        /* insert the new node for this level */
        iStatus = NodeD_new(oPPrefix, oNCurr, &oNNewNode);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNCurr = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNCurr;
        ulIndex++;
    }

    Path_free(oPPath);
    /* update DT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulDirCount += ulNewNodes;

    return SUCCESS;
}   

/* ================================================================== */
/*
  Returns TRUE if the FT contains a directory with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsDir(const char *pcPath) {
    int iStatus;
    NodeD_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findDir(pcPath, &oNFound);
    return (boolean) (iStatus == SUCCESS);
}

/* ================================================================== */
/*
  Removes the FT hierarchy (subtree) at the directory with absolute
  path pcPath. Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_DIRECTORY if pcPath is in the FT as a file not a directory
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmDir(const char *pcPath) {
    int iStatus;
    NodeD_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findDir(pcPath, &oNFound);

    if(iStatus != SUCCESS)
        return iStatus;

    ulDirCount -= NodeD_free(oNFound);
    assert(DynArray_getLength(NodeD_getDirChildren(oNFound)) == ulDirCount);
    if(ulDirCount == 0)
        oNRoot = NULL;

    return SUCCESS;
}

/* ================================================================== */
/*
   Inserts a new  file into the FT with absolute path pcPath, with
   file contents pvContents of size ulLength bytes.
   Returns SUCCESS if the new file is inserted successfully.
   Otherwise, returns:
   * INITIALIZATION_ERROR if the FT is not in an initialized state
   * BAD_PATH if pcPath does not represent a well-formatted path
   * CONFLICTING_PATH if the root exists but is not a prefix of pcPath,
                      or if the new file would be the FT root
   * NOT_A_DIRECTORY if a proper prefix of pcPath exists as a file
   * ALREADY_IN_TREE if pcPath is already in the FT (as dir or file)
   * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_insertFile(const char *pcPath, void *pvContents, size_t 
ulLength) {
    int iStatus;
    Path_T oPPath = NULL; 
    NodeD_T oNFirstNew = NULL;
    NodeD_T oNParent = NULL;
    NodeF_T oNNewFile = NULL;
    size_t ulDepth, ulIndex, ulChildID;
    size_t ulNewNodes = 0; 

    /* validate pcPath and generate a Path_T for it */
    if(!bIsInitialized)
        return INITIALIZATION_ERROR;
    
    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS)
        return iStatus;
    
    if(Path_getDepth(oPPath) == 1)
        return CONFLICTING_PATH;
    
    /* find the closest directory ancestor of oPPath already in the 
    tree, ancestor must be a directory by definition of file tree */
    iStatus= FT_traversePath(oPPath, &oNParent);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        return iStatus;
    }

    /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
    if(oNParent == NULL && oNRoot != NULL) {
        Path_free(oPPath);
        return CONFLICTING_PATH;
    }

    ulDepth = Path_getDepth(oPPath);
    if (oNParent == NULL) /* new root! */
        ulIndex = 1;
    else {
        ulIndex = Path_getDepth(NodeD_getPath(oNParent)) + 1;
        /* the file is already a child of its parent directory */
        if (NodeD_hasFileChild(oNParent, oPPath, &ulChildID) || 
        NodeD_hasDirChild(oNParent, oPPath, &ulChildID) || 
        (Path_comparePath(NodeD_getPath(oNParent), oPPath) == 0)) {
            Path_free(oPPath);
            return ALREADY_IN_TREE;
        }
    }

    /* starting at oNParent, build rest of the directories one by one 
    but not the file itself yet, hence < not <= */
    while (ulIndex < ulDepth) {
        Path_T oPPrefix = NULL;
        NodeD_T oNNewNode = NULL;

        /* generate a Path_T for this level */
        iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
        if (iStatus != SUCCESS) {
            Path_free(oPPath);
            if (oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }
        
        /* If trying to insert child of a file */
        if (FT_containsFile(Path_getPathname(oPPrefix))) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if (oNFirstNew != NULL){
                (void)NodeD_free(oNFirstNew);
            }
            return NOT_A_DIRECTORY;
        }

        /* insert the new node for this level */
        iStatus = NodeD_new(oPPrefix, oNParent, &oNNewNode);
        if(iStatus != SUCCESS) {
            Path_free(oPPath);
            Path_free(oPPrefix);
            if(oNFirstNew != NULL)
                (void) NodeD_free(oNFirstNew);
            return iStatus;
        }

        /* set up for next level */
        Path_free(oPPrefix);
        oNParent = oNNewNode;
        ulNewNodes++;
        if(oNFirstNew == NULL)
            oNFirstNew = oNParent;
        ulIndex++;
    }

    
    iStatus = NodeF_new(oPPath, &oNNewFile);
    if(iStatus != SUCCESS) {
        Path_free(oPPath);
        if(oNFirstNew != NULL)
            (void) NodeD_free(oNFirstNew);
        return iStatus;
    }
    /* MAY NEED TO FREE MEMORY HERE!!! FREE(oNNewFile).*/
    /* ALSO MAYBE PUT THIS ALREADY_IN_TREE STUFF AT THE BEGINNING */
    if (NodeD_hasFileChild(oNParent, oPPath, &ulChildID)) {
        Path_free(oPPath);
        return ALREADY_IN_TREE;
    }
    iStatus = NodeD_addFileChild(oNParent, oNNewFile, ulChildID);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        if(oNFirstNew != NULL)
            (void) NodeD_free(oNFirstNew);
        return iStatus; 
    }
    ulNewNodes++;

    Path_free(oPPath);
    /* update DT state variables to reflect insertion */
    if(oNRoot == NULL)
        oNRoot = oNFirstNew;
    ulDirCount += ulNewNodes;   /* MAKE SURE ulNewNodes ONLY INCLUDES NUMBER OF DIRECTORIES ADDED=======================================*/

    return SUCCESS;
}

/* ================================================================== */
/*
  Returns TRUE if the FT contains a file with absolute path
  pcPath and FALSE if not or if there is an error while checking.
*/
boolean FT_containsFile(const char *pcPath) {
    int iStatus;
    NodeF_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findFile(pcPath, &oNFound);
    return (boolean) (iStatus == SUCCESS);
}

/* ================================================================== */
/*
  Removes the FT file with absolute path pcPath.
  Returns SUCCESS if found and removed.
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root exists but is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * NOT_A_FILE if pcPath is in the FT as a directory not a file
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
int FT_rmFile(const char *pcPath) {
    int iStatus;
    size_t ulIndex;
    NodeF_T oNFound = NULL;
    NodeD_T oNdParent = NULL;

    assert(pcPath != NULL);

    /* Find file and assign to oNFound */
    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return iStatus;

    /* Find parent of the file */    
    iStatus = FT_traversePath(NodeF_getPath(oNFound), &oNdParent);
    if (iStatus != SUCCESS) {
        return iStatus;
    }
    (void)NodeD_hasFileChild(oNdParent, NodeF_getPath(oNFound),
    &ulIndex);

    /* Remove and free the file node */
    NodeF_free(oNFound);
    (void)DynArray_removeAt(NodeD_getFileChildren(oNdParent),ulIndex);

    return SUCCESS;
}

/* ================================================================== */
/*
  Returns the contents of the file with absolute path pcPath.
  Returns NULL if unable to complete the request for any reason.

  Note: checking for a non-NULL return is not an appropriate
  contains check, because the contents of a file may be NULL.
*/
void *FT_getFileContents(const char *pcPath) {
    int iStatus;
    NodeF_T oNFound;

    assert(pcPath != NULL);

    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return NULL;

    return NodeF_getContents(oNFound);
}

/* ================================================================== */
/*
  Replaces current contents of the file with absolute path pcPath with
  the parameter pvNewContents of size ulNewLength bytes.
  Returns the old contents if successful. (Note: contents may be NULL.)
  Returns NULL if unable to complete the request for any reason.
*/
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents, 
size_t ulNewLength) {
    int iStatus;
    NodeF_T oNFound = NULL;

    assert(pcPath != NULL);

    iStatus = FT_findFile(pcPath, &oNFound);
    if(iStatus != SUCCESS)
        return NULL;
    
    (void)NodeF_replaceLength(oNFound,ulNewLength);
    return NodeF_replaceContents(oNFound,pvNewContents);
}

/* ================================================================== */
/*
  Returns SUCCESS if pcPath exists in the hierarchy,
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * MEMORY_ERROR if memory could not be allocated to complete request

  When returning SUCCESS,
  if path is a directory: sets *pbIsFile to FALSE, *pulSize unchanged
  if path is a file: sets *pbIsFile to TRUE, and
                     sets *pulSize to the length of file's contents

  When returning another status, *pbIsFile and *pulSize are unchanged.
*/
int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    NodeD_T oNdFound = NULL;
    NodeF_T oNfFound = NULL;
    int iStatusDir;
    int iStatusFile;

    iStatusDir = FT_findDir(pcPath, &oNdFound);
    iStatusFile = FT_findFile(pcPath, &oNfFound);

    if (iStatusDir == SUCCESS) {
        *pbIsFile = (int) FALSE;
        return iStatusDir;
    }
    else if (iStatusFile == SUCCESS) {
        *pbIsFile = (int) TRUE;
        *pulSize = NodeF_getLength(oNfFound);
        return iStatusFile;
    }
    else {
        return iStatusDir;
    }
}

/* ================================================================== */
/*
  Sets the FT data structure to an initialized state.
  The data structure is initially empty.
  Returns INITIALIZATION_ERROR if already initialized,
  and SUCCESS otherwise.
*/
int FT_init(void) {
    if(bIsInitialized)
        return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulDirCount = 0;

    return SUCCESS;
}

/* ================================================================== */
/*
  Removes all contents of the data structure and
  returns it to an uninitialized state.
  Returns INITIALIZATION_ERROR if not already initialized,
  and SUCCESS otherwise.
*/
int FT_destroy(void) {
    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    if(oNRoot) {
        ulDirCount -= NodeD_free(oNRoot);
        oNRoot = NULL;
    }

    assert(ulDirCount == 0);

    bIsInitialized = FALSE;

    return SUCCESS;
}

/* ================================================================== */
/*
  The following auxiliary functions are used for generating the
  string representation of the FT.
*/

/*
  Performs pre-order traversal of file tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/

static size_t FT_preOrderTraversal(NodeD_T n, DynArray_T d, size_t i) {
    size_t c;

    assert(d != NULL);

    if(n != NULL) {
        (void) DynArray_set(d, i, n);
        i++;
        for(c = 0; c < NodeD_getNumDirChildren(n); c++) {
            int iStatus;
            NodeD_T oNdChild = NULL;
            iStatus = NodeD_getDirChild(n,c, &oNdChild);
            assert(iStatus == SUCCESS);
            i = FT_preOrderTraversal(oNdChild, d, i);
        }
    }
    return i;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNdNode's path, and also always adds one addition byte to the sum.
  Also accumulates the lengths of oNdNode's children Paths.
*/
static void FT_strlenAccumulate(NodeD_T oNdNode, size_t *pulAcc) {
    char* pcNodeString;

    assert(pulAcc != NULL);

    if(oNdNode != NULL) {
        pcNodeString = NodeD_toString(oNdNode);
        *pulAcc += strlen(pcNodeString);
        free(pcNodeString);      
    }
}

/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNdNode's string represenation onto pcAcc, and also 
  always adds one newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(NodeD_T oNdNode, char *pcAcc) {
    char* pcNodeString;

    assert(pcAcc != NULL);

    if(oNdNode != NULL) {
        pcNodeString = NodeD_toString(oNdNode);
        strcat(pcAcc, pcNodeString);
        free(pcNodeString);
    }
}

/* ================================================================== */
char *FT_toString(void) {
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;

    if(!bIsInitialized)
        return NULL;

    nodes = DynArray_new(ulDirCount);
    (void) FT_preOrderTraversal(oNRoot, nodes, 0);

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                    (void*) &totalStrlen);

    result = malloc(totalStrlen + 1);
    if(result == NULL) {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
    (void *) result);
    
    DynArray_free(nodes);

    return result;
}