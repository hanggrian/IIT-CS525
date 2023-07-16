#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"
#include "storage_mgr.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
	RS_FIFO = 0,
	RS_LRU = 1,
	RS_CLOCK = 2,
	RS_LFU = 3,
	RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
	char *pageFile; // the name of the page file associated with the buffer pool
	int numPages; // the number of page frames
	ReplacementStrategy strategy; // the page replacement strategy
	void *mgmtData; // use this one to store the bookkeeping info your buffer
	// manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
	PageNumber pageNum; // position of the page in the page file, the first data page in a page file is 0
	char *data; // points to the area in memory storing the content of the page
} BM_PageHandle;


// Page Frame: each array entry in buffer pool
typedef struct Frame {
	PageNumber pageNum; // which page is currently stored in the frame
	int pinCount; // how many processes are using this page
	int dirtyBit; // whether the page has been modified
	char* data; // points to the area in memory storing the content of the page
}Frame;

// used by LRU
typedef struct Hash
{
    int capacity;
	int pageCnt;
    Frame* *arr;
} Hash;

// The cached page information
typedef struct PageCache {
	int front;
	int rear;
	int frameCnt; // the number of used frames in this buffer pool
	int capacity; // the total number of frames the page cache can store
	Frame* *arr; // store frames information
	//add by Jessica
	int numRead; //stores number of pages that have been read
	int numWrite; //stores number of pages that been written
	// to solve segment default issue by store the file handle
	SM_FileHandle* fHandle;
	// hash for LRU
	int* hash; // store the frames information
}PageCache;


// convenience macros
#define MAKE_POOL()					\
		((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
		((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

// Helper Interface
// manamge resources in buffer pool
Frame* createFrameNode();
void* resetFrameNode(Frame* frame);
int* createHash(int capacity);
PageCache* createPageCache(BM_BufferPool *const bm, int numPages);
void freeFrame(PageCache* pageCache);
void freeFileHandle(PageCache* pageCache);
void freeHash(PageCache* pageCache);
void freePageCache(PageCache* pageCache);

// Manage PageCache in buffer pool
int isFull(PageCache* pageCache);
int isEmpty(PageCache* pageCache);
Frame* isHitPageCache(PageCache* pageCache, const PageNumber pageNum);
RC addPageToPageCacheWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,
				int pageNum);
RC addPageToPageCacheWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum);
RC updateLRUOrder(PageCache* pageCache, int pageNum);
RC removePageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page);
Frame* removePageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, int leastUsedPage);
Frame* searchPageFromCache(PageCache *const pageCache, int pageNum);

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		const int numPages, ReplacementStrategy strategy,
		void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
int *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

#endif
