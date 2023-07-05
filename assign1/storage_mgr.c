#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "dberror.h"

// Instantiate the storage manager by printing a message to standard out.
void initStorageManager(void) {
  printf("The program begins to initialize storage manager.\n");
}

// The createPageFile function is to create a new page file with one page size.
// This page file fills with '\0' bytes.
RC createPageFile(char *fileName) {
  // validates parameters
  if (fileName == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // creates a non-null file pointer
  FILE *fp = fopen(fileName, "w+");
  if (fp == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // allocates the PAGE_SIZE memory and assigns the value to str pointer
  char *str = (char *) calloc(PAGE_SIZE, sizeof(char));
  fwrite(str, sizeof(char), PAGE_SIZE, fp);

  // closes file, flushes buffers, deallocates memory
  fclose(fp);
  free(str);
  return RC_OK;
}

// The openPageFile function is to open an existing file and get statistic data
// and store those to the file handle.
//
// - If the file doesn't exist, return RC_FILE_NOT_FOUND.
// - If opening the file is successful, then the files of this file handle is
//   initialized with the information about the opened file.
//
// For example, the information about the opened file may contain file name,
// total number of pages, current page position
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
  // validates parameters
  if (fileName == NULL) {
    return RC_FILE_NOT_FOUND;
  }
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // opens a non-null file pointer
  FILE *fp = fopen(fileName, "r+");
  if (fp == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // seek to the end of the file to get file size, measure total pages
  if (fseek(fp, 0, SEEK_END) != 0) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // stores file information, reset position
  fHandle->mgmtInfo = fp;
  fHandle->fileName = fileName;
  fHandle->curPagePos = 0;

  // measure total pages
  long fileSize = ftell(fp);
  fHandle->totalNumPages = (int) (fileSize / PAGE_SIZE);
  return RC_OK;
}

// The closePageFile method is to close the current page file, removing every
// reference to the file.
RC closePageFile(SM_FileHandle *fHandle) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // get the file pointer through fHandle
  FILE *fp = fHandle->mgmtInfo;
  fclose(fp);
  if (fp != NULL) {
    fp = NULL;
  }
  return RC_OK;
}

// The destroyPageFile method is to delete the page file based on filename.
RC destroyPageFile(char *fileName) {
  // validates parameters
  if (fileName == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // if the result is not zero, then some errors happened in this process
  if (remove(fileName) != 0) {
    return RC_FILE_NOT_FOUND;
  }
  return RC_OK;
}

/* reading blocks from disc */

// The readBlock method is to read the pageNum block from a file and stores its
// content in the memory pointed to by the memPage page handle.
//
// - If the file has less than pageNum pages, the method should
//   return RC_READ_NON_EXISTING_PAGE.
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }
  if (memPage == NULL) {
    return RC_WRITE_FAILED;
  }

  // check the validation of pageNum, since the pageNum starts with 0, so the
  // valid range of pageNum should be range of totalNumPages
  if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // get the non-null file pointer
  FILE *fp = fHandle->mgmtInfo;
  if (fp == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // seek to the offset of the file
  int offset = pageNum * PAGE_SIZE;
  if (fseek(fp, offset, SEEK_SET) != 0) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // read data from memory, update page
  if (fread(memPage, sizeof(char), PAGE_SIZE, fp) < PAGE_SIZE) {
    return RC_READ_NON_EXISTING_PAGE;
  }
  fHandle->curPagePos = pageNum;
  return RC_OK;

}

// The getBlockPos method is to get the current page position in a file.
int getBlockPos(SM_FileHandle *fHandle) {
  // validates parameters
  if (fHandle == NULL) {
    return -1;
  }
  return fHandle->curPagePos;
}

// The readFirstBlock method is to read the first page in a file. The current
// page position should be moved to the page that was read.
//
// - If the user tries to read a block before the first page or after the last
//   page of the file, the method should return RC READ NON EXISTING PAGE.
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // the first block number is page 0
  return readBlock(0, fHandle, memPage);
}

// The readPreviousBlock method is to read the previous page relative to the
// current page position of the file. The current page position should be moved
// to the page that was read.
//
// - If the user tries to read a block before the first page or after the last
//   page of the file, the method should return RC_READ_NON_EXISTING_PAGE.
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // if the value of position is -1, errors happened in this process
  int pageNum = getBlockPos(fHandle);
  if (pageNum == -1) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // the previous block should be pageNum - 1
  return readBlock(pageNum - 1, fHandle, memPage);
}

// The readCurrentBlock method is to read the current page position of the file.
// The current page position should be moved to the page that was read.
//
// - If the user tries to read a block before the first page or after the last
//   page of the file, the method should return RC_READ_NON_EXISTING_PAGE.
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // if the value of pageNum is -1, errors happened in this process
  int pageNum = getBlockPos(fHandle);
  if (pageNum == -1) {
    return RC_FILE_HANDLE_NOT_INIT;
  }
  return readBlock(pageNum, fHandle, memPage);

}

// The readNextBlock method is to read the next page relative to the current
// page position of the file. The current page position should be moved to the
// page that was read.
//
// - If the user tries to read a block before the first page or after the last
//   page of the file, the method should return RC_READ_NON_EXISTING_PAGE.
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // if the value of pageNum is -1, errors happened in this process
  int pageNum = getBlockPos(fHandle);
  if (pageNum == -1) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // the next block should be pageNum + 1
  return readBlock(pageNum + 1, fHandle, memPage);
}

// The readLastBlock method is to read the last page in a file.
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // the previous block should be pageNum - 1
  int lastBlockPageNum = fHandle->totalNumPages - 1;
  return readBlock(lastBlockPageNum, fHandle, memPage);
}

/* writing blocks to a page file */

// The writeBlock method is to write a page date to disk using either the
// current position or an absolute position.
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }
  if (memPage == NULL) {
    return RC_WRITE_FAILED;
  }
  if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // get the non-null file pointer
  FILE *fp = fHandle->mgmtInfo;
  if (fp == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // seek to the offset of the file
  int offset = pageNum * PAGE_SIZE;
  if (fseek(fp, offset, SEEK_SET) != 0) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // write data from memory, update page
  if (fwrite(memPage, sizeof(char), strlen(memPage), fp) < PAGE_SIZE) {
    return RC_WRITE_FAILED;
  }
  fHandle->curPagePos = pageNum;
  return RC_OK;
}

// The writeCurrentBlock method is to write current page to disk using either
// the current position or an absolute position.
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // write block using current page number
  int curPageNum = fHandle->curPagePos;
  return writeBlock(curPageNum, fHandle, memPage);
}

// The appendEmptyBlock method is to increase the number of pages in the file by
// one. The new last page should be filled with zero bytes.
RC appendEmptyBlock(SM_FileHandle *fHandle) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }

  // get the non-null file pointer
  FILE *fp = fHandle->mgmtInfo;
  if (fp == NULL) {
    return RC_FILE_NOT_FOUND;
  }

  // move pointer to the end of the file
  if (fseek(fp, 0, SEEK_END) != 0) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // allocates the PAGE_SIZE memory and assigns the value to str pointer
  char *str = (char *) calloc(PAGE_SIZE, sizeof(char));
  if (fwrite(str, sizeof(char), PAGE_SIZE, fp) < PAGE_SIZE) {
    free(str);
    return RC_WRITE_FAILED;
  }

  // plus 1 to total number of pages, deallocates the memory
  fHandle->totalNumPages++;
  free(str);
  return RC_OK;
}

// The ensureCapacity method is to check current capacity.
//
// - If the file has less than number of pages, then increase the size to the
//   number of pages.
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
  // validates parameters
  if (fHandle == NULL) {
    return RC_FILE_HANDLE_NOT_INIT;
  }
  if (numberOfPages < 1) {
    return RC_READ_NON_EXISTING_PAGE;
  }

  // append remaining blocks
  int currentNumPages = fHandle->totalNumPages;
  int cnt = numberOfPages - currentNumPages;
  for (int i = 0; i < cnt; i++) {
    appendEmptyBlock(fHandle);
  }

  // check whether the total number of pages is the same as required
  if (fHandle->totalNumPages != numberOfPages) {
    return RC_WRITE_FAILED;
  }
  return RC_OK;
}
