#include "storage_mgr.h"

void initStorageManager(void) {
}

RC createPageFile(char *fileName) {
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
}

RC closePageFile(SM_FileHandle *fHandle) {
}

RC destroyPageFile(char *fileName) {
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

int getBlockPos(SM_FileHandle *fHandle) {
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
}
