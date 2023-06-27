#include "storage_mgr.h"
#include <iostream>

void initStorageManager() {
}

RC createPageFile(char *fileName) {
}

RC openPageFile(char *fileName, SmFileHandle *fileHandle) {
}

RC closePageFile(SmFileHandle *fileHandle) {
}

RC destroyPageFile(char *fileName) {
}

RC readBlock(int page, SmFileHandle *fileHandle,
             SmPageHandle pageHandle) {
}

int getBlockPos(SmFileHandle *fileHandle) {
}

RC readFirstBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC readLastBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC readPreviousBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC readCurrentBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC readNextBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC writeBlock(int page, SmFileHandle *fileHandle,
              SmPageHandle pageHandle) {
}

RC writeCurrentBlock(SmFileHandle *fileHandle, SmPageHandle pageHandle) {
}

RC appendEmptyBlock(SmFileHandle *fileHandle) {
}

RC ensureCapacity(int maxPage, SmFileHandle *fileHandle) {
}
