#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <cstdio>
#include "dberror.h"

// A file handle represents an open page file.
struct SmFileHandle {
  char *fileName;
  int totalPages;
  int currentPage;
  void *properties;
};

// A page handle is a pointer to an area in memory storing the data of a page.
typedef char *SmPageHandle;

extern void initStorageManager();

/* File related methods */
extern RC createPageFile(char *fileName);
extern RC openPageFile(char *fileName, SmFileHandle *fileHandle);
extern RC closePageFile(SmFileHandle *fileHandle);
extern RC destroyPageFile(char *fileName);

/* Read and write methods */
extern RC readBlock(int page, SmFileHandle *fileHandle,
                    SmPageHandle pageHandle);
extern int getBlockPos(SmFileHandle *fileHandle);
extern RC readFirstBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC readLastBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC readPreviousBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC readCurrentBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC readNextBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC writeBlock(int page, SmFileHandle *fileHandle,
                     SmPageHandle pageHandle);
extern RC writeCurrentBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle);
extern RC appendEmptyBlock(SmFileHandle *fileHandle);
extern RC ensureCapacity(int maxPage, SmFileHandle *fileHandle);

#endif
