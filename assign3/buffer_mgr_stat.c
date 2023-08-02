// This file implements interfaces related to Statistics Buffer
// defined in buffer_mgr.h header.

#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"

#include <stdio.h>
#include <stdlib.h>

// local functions
static void printStrat (BM_BufferPool *const bm);

// external functions
void 
printPoolContent (BM_BufferPool *const bm)
{
	PageNumber *frameContent;
	int *dirty;
	int *fixCount;
	int i;

	frameContent = getFrameContents(bm);
	dirty = getDirtyFlags(bm);
	fixCount = getFixCounts(bm);

	printf("{");
	printStrat(bm);
	printf(" %i}: ", bm->numPages);

	for (i = 0; i < bm->numPages; i++)
		printf("%s[%i%s%i]", ((i == 0) ? "" : ",") , frameContent[i], (dirty[i] ? "x": " "), fixCount[i]);
	printf("\n");
	free(frameContent);
	free(dirty);
	free(fixCount);
}

char *
sprintPoolContent (BM_BufferPool *const bm)
{
	PageNumber *frameContent;
	int *dirty;
	int *fixCount;
	int i;
	char *message;
	int pos = 0;

	message = (char *) malloc(256 + (22 * bm->numPages));
	frameContent = getFrameContents(bm);
	dirty = getDirtyFlags(bm);
	fixCount = getFixCounts(bm);

	for (i = 0; i < bm->numPages; i++)
		pos += sprintf(message + pos, "%s[%i%s%i]", ((i == 0) ? "" : ",") , frameContent[i], (dirty[i] ? "x": " "), fixCount[i]);
	free(frameContent);
	free(dirty);
	free(fixCount);
	return message;
}


void
printPageContent (BM_PageHandle *const page)
{
	int i;

	printf("[Page %i]\n", page->pageNum);

	for (i = 1; i <= PAGE_SIZE; i++)
		printf("%02X%s%s", page->data[i], (i % 8) ? "" : " ", (i % 64) ? "" : "\n");
}

char *
sprintPageContent (BM_PageHandle *const page)
{
	int i;
	char *message;
	int pos = 0;

	message = (char *) malloc(30 + (2 * PAGE_SIZE) + (PAGE_SIZE % 64) + (PAGE_SIZE % 8));
	pos += sprintf(message + pos, "[Page %i]\n", page->pageNum);

	for (i = 1; i <= PAGE_SIZE; i++)
		pos += sprintf(message + pos, "%02X%s%s", page->data[i], (i % 8) ? "" : " ", (i % 64) ? "" : "\n");

	return message;
}

void
printStrat (BM_BufferPool *const bm)
{
	switch (bm->strategy)
	{
	case RS_FIFO:
		printf("FIFO");
		break;
	case RS_LRU:
		printf("LRU");
		break;
	case RS_CLOCK:
		printf("CLOCK");
		break;
	case RS_LFU:
		printf("LFU");
		break;
	case RS_LRU_K:
		printf("LRU-K");
		break;
	default:
		printf("%i", bm->strategy);
		break;
	}
}

// Statistics Interface

// The getFrameContents function returns an array of PageNumbers (of size numPages) 
// where the ith element is the number of the page stored in the ith page frame. 
// An empty page frame is represented using the constant NO PAGE.
PageNumber *getFrameContents (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return NULL;
	}

	int totalNumPages = bm->numPages;

	PageCache* pageCache = bm->mgmtData;
	
	PageNumber *arr = (PageNumber*) malloc(bm->numPages * sizeof(PageNumber));
	int i;
	for(i = 0; i < bm->numPages;i++) {
		arr[i] = pageCache->arr[i]->pageNum;
	}
	return arr;

}

// The getDirtyFlags function returns an array of bools (of size numPages) where the ith element
// is TRUE if the page stored in the ith page frame is dirty. Empty page frames are considered as clean.
int *getDirtyFlags (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return NULL;
	}

	PageCache* pageCache = bm->mgmtData;
	int numPages = bm->numPages;
	int *arr = (PageNumber*) malloc(numPages * sizeof(int));

	int i;
	for(int i = 0; i < numPages; i++) {
		arr[i] = pageCache->arr[i]->dirtyBit;
	}
	return arr;


}

// The getFixCounts function returns an array of ints (of size numPages) where the ith element is
// the fix count of the page stored in the ith page frame. Return 0 for empty page frames.
int *getFixCounts (BM_BufferPool *const bm)
{
	if(bm == NULL) {
		return NULL;
	}

	PageCache* pageCache = bm->mgmtData;
	int numPages = bm->numPages;
	int *arr = (PageNumber*) malloc(numPages * sizeof(int));

	int i;
	for(int i = 0; i < numPages; i++) {
		arr[i] = pageCache->arr[i]->pinCount;
	}
	return arr;

}
int getNumReadIO (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return -1;
	}
	// get page cache 
    PageCache* pageCache = bm->mgmtData;

	return pageCache->numRead;
}
int getNumWriteIO (BM_BufferPool *const bm) {
	if(bm == NULL) {
		return -1;
	}
	// get page cache 
    PageCache* pageCache = bm->mgmtData;

	return pageCache->numWrite;
}
