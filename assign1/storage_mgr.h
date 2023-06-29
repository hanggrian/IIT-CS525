#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "dberror.h"

// A file handle represents an open page file.
typedef struct SM_FileHandle {
  char *fileName;
  int totalNumPages;
  int curPagePos;
  void *mgmtInfo;
} SM_FileHandle;

// A page handle is a pointer to an area in memory storing the data of a page.
typedef char *SM_PageHandle;

extern void initStorageManager(void);

/* File related methods */
extern RC createPageFile(char *fileName);
extern RC openPageFile(char *fileName, SM_FileHandle *fHandle);
extern RC closePageFile(SM_FileHandle *fHandle);
extern RC destroyPageFile(char *fileName);

/* Read and write methods */
extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
extern int getBlockPos(SM_FileHandle *fHandle);
extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle,
                     SM_PageHandle memPage);
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock(SM_FileHandle *fHandle);
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle);

#endif
