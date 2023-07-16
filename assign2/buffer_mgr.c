// This file implements interfaces related to Pool Handling and Access Page
//  defined in buffer_mgr.h header.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"


// initBufferPool creats a new buffer pool with numPages page frames using the page replacement strategy.
// The pool is used to cache pages from the page file with name pageFileName.
// -- Initially, all page frames should be empty.
// -- The page file should already exist.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                    const int numPages, ReplacementStrategy strategy,
		            void *stratData)
{
    // check the validation of parameters
    if(bm == NULL || pageFileName == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // the number of page frames in this buffer pool
    if (numPages <= 0) {
        return RC_ERROR;
    }

    // check if the file specified by the filename exisits
    FILE *fp = fopen(pageFileName, "r+");
    if(fp == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    // initialzie values of a new buffer pool
    bm->pageFile = (char *) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    // initialize page cache
    PageCache* pageCache = createPageCache(bm, numPages);

    bm->mgmtData = pageCache;

    fclose(fp);

    return RC_OK;

}

// shutdownBufferPool is to destory a buffer pool.
// The method frees up all resources associated with buffer pool.
// -- Free the memory allocated for page frames.
// -- If the buffer pool contains any dirty pages, then these pages should be written back to disk before destroying.
// -- Raise an errot if a buffer pool has pinned pages.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    // check validation of bm
    if(bm == NULL) {
        return RC_ERROR;
    }

    // get current page cacha
    PageCache* pageCache = bm->mgmtData;

    if(pageCache == NULL) {
        return RC_OK;
    }

    // force to flush all pages in buffer pool
    if(forceFlushPool(bm) != RC_OK) {
        return RC_ERROR;
    }

    // release all resources assigned to page cache
    freePageCache(pageCache);

    bm->mgmtData = NULL;

    return RC_OK;

}


// forceFlushPool is to cause all dirty pages from the buffer pool to be written to disk
// -- check whether there are dirty pages as well as the pin counts is equal to 0
RC forceFlushPool(BM_BufferPool *const bm)
{
    // check validation of bm
    if(bm == NULL) {
        return RC_ERROR;
    }

    // get the store the page cache
    PageCache* pageCache = bm->mgmtData;

    if(pageCache == NULL) {
        return RC_OK;
    }
    // iterate to check all frames
    int i;
    for(i = 0; i < pageCache->capacity; i++) {
        Frame* frame = pageCache->arr[i];
        // this frame has no page file
        if(frame->pageNum == NO_PAGE) {
            continue;
        }
        // force all drity pages from the buffer pool to be written to disk
        if (frame->dirtyBit == 1 && frame->pinCount == 0) {
            // get the disk page handle pointer
            SM_FileHandle *fHandle = pageCache->fHandle;

            // write this dirty page to the disk
            if(writeBlock(frame->pageNum, fHandle, frame->data) != RC_OK) {
                return RC_WRITE_FAILED;
            }
            pageCache->numWrite++;

            // after flush all dirth pages in buffer pool
            frame->dirtyBit = 0;
        }
    }

    return RC_OK;
}


// Buffer Manager Interface Access Pages

// pinPage is to pin the page with page number pageNum.
// pinning a page means that clients of the buffer mananger can request this page number.
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum)
{
    // check validations of parameters
    if(bm == NULL || page == NULL || pageNum < 0) {
        return RC_ERROR;
    }

    // get the pageCache in this buffer
    PageCache* pageCache = bm->mgmtData;

    // checke whether we can get right pageCache
    if(pageCache == NULL) {
        return RC_ERROR;
    }

    // check whether this pageNum hit the pageCache
    Frame* frame = isHitPageCache(pageCache, pageNum);

    // if yes, hit page cache
    if(frame != NULL) {
        page->pageNum = pageNum;
        page->data = frame->data;
        frame->pinCount++;
        if(bm->strategy == RS_LRU) {
            updateLRUOrder(pageCache, pageNum);
        }
        return RC_OK;
    }

    // if no execute different pin page processes based on replacement strategy
    if(bm->strategy == RS_FIFO) {
        return addPageToPageCacheWithFIFO(bm, page, pageNum);
    } else if(bm->strategy == RS_LRU) {
        return addPageToPageCacheWithLRU(bm, page, pageNum);
    }
    return RC_OK;
}


// make a page as dirty
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check validation of parameters
    if(bm == NULL || page == NULL) {
        return RC_ERROR;
    }

    // get page cache
    PageCache* pageCache = bm->mgmtData;

    if(pageCache == NULL) {
        return RC_OK;
    }

    // search a frame from page cache
    Frame* frame = searchPageFromCache(pageCache, page->pageNum);

    // if this frame doesn't exist
    if(frame == NULL) {
        return RC_ERROR;
    }

    frame->dirtyBit = 1;

    return RC_OK;
}

// unpins the page.
// The pageNum field of page is used to figure out which page to pin.
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check the validation of parameters
    if(bm == NULL || page == NULL) {
        return RC_ERROR;
    }
    // get page cache
    PageCache* pageCache = bm->mgmtData;

    if(pageCache == NULL) {
        return RC_OK;
    }

    // search a frame from page cache
    Frame* frame = searchPageFromCache(pageCache, page->pageNum);

    // if this frame doesn't exist
    if(frame == NULL) {
        return RC_ERROR;
    }

    frame->pinCount--;

    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
    }
    return RC_OK;

}

// forcePage is to write the current content of page back to the page file on disk.
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    // check the validation of parameters
    if(bm == NULL || page == NULL) {
        return RC_ERROR;
    }

    // get page cache
    PageCache* pageCache = bm->mgmtData;

    if(pageCache == NULL) {
        return RC_OK;
    }

    // search a frame from page cache
    Frame* frame = searchPageFromCache(pageCache, page->pageNum);

    // if this frame doesn't exist
    if(frame == NULL) {
        return RC_ERROR;
    }

    // get the disk page handle pointer
    SM_FileHandle* fHandle = pageCache->fHandle;

    // write this dirty page to the disk
    if(writeBlock(frame->pageNum, fHandle, frame->data) != RC_OK) {
        return RC_WRITE_FAILED;
    }

    return RC_OK;
}


// initialize a new frame node in buffer pool
Frame* createFrameNode()
{
    // allocate memory for this frame
    Frame* frame = (Frame*)calloc(1, sizeof(Frame));

    // allocate memory for storing the content of the page
    char* data = (char *) calloc(PAGE_SIZE, sizeof(char));

    // initialize values for every attributes
    frame->pageNum = NO_PAGE;
    frame->pinCount = 0;
    frame->dirtyBit = 0;
    frame->data = data;
    return frame;
}

//reset this new frame node when remove this frame from buffer pool.
void* resetFrameNode(Frame* frame) {
    frame->pageNum = NO_PAGE;
    frame->pinCount = 0;
    frame->dirtyBit = 0;
}

// create a map to record the utility of every frames, used for LRU
int* createHash(int capacity )
{
    int* hash =(int*)malloc(capacity * sizeof(int));
    for(int i = 0; i < capacity; i++) {
        hash[i] = -1;
    }
    return hash;
}

// create a cache area for pages
PageCache* createPageCache(BM_BufferPool *const bm, int numPages) {
    // allocate memory for this page cache
    PageCache* pageCache = (PageCache* ) malloc(sizeof(PageCache));

    // initialize values for every attribute
    pageCache->front = 0;
    pageCache->rear = -1;
    pageCache->frameCnt = 0;
    pageCache->capacity = numPages;
    pageCache->numRead=0;
    pageCache->numWrite=0;

    // store a page data
    pageCache->arr = (Frame**) malloc(numPages * sizeof(Frame*));
    int i;
    for(i = 0; i < pageCache->capacity; ++i ) {
        Frame* frame = createFrameNode();
        pageCache->arr[i] = frame;
    }

    // store file handle data
    SM_FileHandle* fHandle = (SM_FileHandle*)calloc(1, sizeof(SM_FileHandle));

    openPageFile(bm->pageFile, fHandle);

    pageCache->fHandle = fHandle;

    // initialize hash map
    if(bm->strategy == RS_LRU) {
        pageCache->hash = createHash(numPages);
    } else if(bm->strategy == RS_FIFO) {
        pageCache->hash = NULL;
    }
    return pageCache;
}

// release all resources assigned to frames
void freeFrame(PageCache* pageCache) {
    if(pageCache->arr) {
        for(int i = 0; i < pageCache->capacity; i++) {
            Frame* frame = pageCache->arr[i];
            // release the resources assigned to store the content of the page
            if(frame->data != NULL) {
                free(frame->data);
            }
            free(frame);
            pageCache->arr[i] = NULL;
        }
        free(pageCache->arr);
    }
}

// release the resources assigned to the storage file handle.
void freeFileHandle(PageCache* pageCache) {
    if(pageCache->fHandle) {
        free(pageCache->fHandle);
    }
}

// release map resources assigned to frames created by LRU strategy
void freeHash(PageCache* pageCache) {
    if(pageCache->hash) {
        free(pageCache->hash);
    }
}
void freePageCache(PageCache* pageCache) {
    if(pageCache != NULL) {
        freeFileHandle(pageCache);
        freeFrame(pageCache);
        freeHash(pageCache);
        free(pageCache);
    }
}


// page cache is full when the frameCnt becomes equal to size
int isFull(PageCache* pageCache)
{
    return (pageCache->frameCnt == pageCache->capacity);
}

// page cache is empty when frameCnt is 0
int isEmpty(PageCache* pageCache)
{
    return (pageCache->frameCnt == 0);
}

// check whether the required pageNum hits the cache
Frame* isHitPageCache(PageCache* pageCache, const PageNumber pageNum) {
    // iterate all frames stored in this page cache
    int i;
    for(int i = 0; i < pageCache->capacity; i++) {
        Frame* frame = pageCache->arr[i];
        if(frame->pageNum == pageNum) {
            return frame;
        }
    }
    // the page cache didn't contain the current page number data, return NULL
    return NULL;
}

RC updateLRUOrder(PageCache* pageCache, int pageNum)
{
    int* hash = pageCache->hash;
    // update hash
    int index = -1;
    int i;
    for(i = 0; i < pageCache->capacity; i++) {
        if(hash[i] == pageNum) {
            index = i;
            break;
        }
    }
    if(index == -1) {
        return RC_ERROR;
    }
    int updatePageNum = hash[index];
    for(i = index; i < pageCache->capacity - 1; i++) {
        hash[i] = hash[i + 1];
    }
    hash[pageCache->capacity - 1] = updatePageNum;
    return RC_OK;
}

// add a new frame to pageCache
RC addPageToPageCacheWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page, int pageNum)
{
    // get current page cache
    PageCache* pageCache = bm->mgmtData;

    // The following process is to add this new page to page cache

    // if current page cache is full
    if (isFull(pageCache)) {
        removePageWithFIFO(bm, page);
    }
    // get the frame to store this page content
    pageCache->rear = (pageCache->rear + 1) % pageCache->capacity;

    Frame* frame = pageCache->arr[pageCache->rear];

    // copy the file content from disk to memory
    SM_FileHandle *fHandle = pageCache->fHandle;

    if(ensureCapacity(pageNum + 1, fHandle) != RC_OK) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    if(readBlock(pageNum, fHandle, frame->data) != RC_OK) {
        return RC_ERROR;
    }

    pageCache->numRead++;

    // update this frame information page
    frame->pageNum = pageNum;
    frame->pinCount = 1;
    frame->dirtyBit = 0;

    // store page number info to page
    page->pageNum = pageNum;
    page->data = frame->data;

    // store this page in the cache
    // pageCache->arr[pageCache->rear] = frame;
    pageCache->frameCnt = pageCache->frameCnt + 1;

    return RC_OK;
}

// add new page to page cache based on LRU strategy
RC addPageToPageCacheWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page,
		const PageNumber pageNum)
{
    // get current page cache
    PageCache* pageCache = bm->mgmtData;

    int* hash = pageCache->hash;

    int frameIndex = -1;

    Frame* frame = NULL;
    // mark whether the current pageCache is full
    int fullFlag = 0;
    if(isFull(pageCache)) {
        int leastUsedPageNum = hash[0];
        frame = removePageWithLRU(bm, page, leastUsedPageNum);
        fullFlag = 1;
        // frame = pageCache->arr[leastUsedPageNum];
    } else {
        for(int i = 0; i < bm->numPages; i++) {
            if(hash[i] == -1) {
                frameIndex = i;
                break;
            }
        }
        if(frameIndex == -1) {
            return RC_ERROR;
        }
        frame = pageCache->arr[frameIndex];
    }

    if(frame == NULL) {
        return RC_ERROR;
    }

    // copy the page content
    SM_FileHandle *fHandle = pageCache->fHandle;

    // ensure the file page exists
    if(ensureCapacity(pageNum + 1, fHandle) != RC_OK) {
        return RC_READ_NON_EXISTING_PAGE;
    }

    // copy the file content from disk to memory
    if(readBlock(pageNum, fHandle, frame->data) != RC_OK) {
        return RC_ERROR;
    }

    // print frame page
    // printf("current fram index = %d\n", frameIndex);

    pageCache->numRead++;

    // update this frame information page
    frame->pageNum = pageNum;
    frame->pinCount = 1;
    frame->dirtyBit = 0;

    // store page number info to page
    page->pageNum = pageNum;
    page->data = frame->data;

    pageCache->frameCnt = pageCache->frameCnt + 1;

    // store this page in the cache
    if(fullFlag == 1) {
        // pageCache->arr[hash[0]] = frame;
        for(int i = 0; i < pageCache->capacity - 1; i++) {
            hash[i] = hash[i+1];
        }
        hash[pageCache->capacity - 1] = pageNum;
    } else {
        // pageCache->arr[frameIndex] = frame;
        hash[frameIndex] = pageNum;
    }

    return RC_OK;
}

// Remove a frame from queue based on FIFO. It changes front and frameCnt
RC removePageWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageCache* pageCache = bm->mgmtData;
    // check whether this page cache is empty
    if (isEmpty(pageCache))
        return RC_ERROR;

    // check whether there exisit frame with pinCount = 0
    int cnt = 0;
    Frame** arr = pageCache->arr;
    for(int i = 0; i < pageCache->capacity; i++) {
        if(arr[i]->pinCount == 0) {
            cnt++;
        }
    }
    if(cnt == 0) {
        return RC_ERROR;
    }

    // get the first frame in the page cache
    Frame* frame = pageCache->arr[pageCache->front];

    // fix test case :201
    if(frame->pinCount > 0) {
        while(pageCache->arr[pageCache->front]->pinCount > 0) {
            pageCache->front = (pageCache->front + 1) % pageCache->capacity;
        }
        frame = pageCache->arr[pageCache->front];

        // set the tail to the current frame
        pageCache->rear = pageCache->front - 1;
    }
    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
        pageCache->numWrite++;
    }
    // remove the first frame
    pageCache->front = (pageCache->front + 1) % pageCache->capacity;

    // update the number of used frame in page cache
    pageCache->frameCnt = pageCache->frameCnt - 1;

    // reset this frame node
    resetFrameNode(frame);

    return RC_OK;
}

Frame* removePageWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page, int leastUsedPage)
{
    PageCache* pageCache = bm->mgmtData;
    // check whether this page cache is empty
    if (isEmpty(pageCache))
        return NULL;

    // check whether there exisit frame with pinCount = 0
    int cnt = 0;
    Frame** arr = pageCache->arr;
    for(int i = 0; i < pageCache->capacity; i++) {
        if(arr[i]->pinCount == 0) {
            cnt++;
        }
    }
    if(cnt == 0) {
        return NULL;
    }

    // get the least page in the page cache
    Frame* frame = searchPageFromCache(pageCache, leastUsedPage);

    if(frame == NULL) {
        return NULL;
    }

    if(frame->pinCount == 0 && frame->dirtyBit == 1) {
        forcePage(bm, page);
        pageCache->numWrite++;
    }

    // remove the least page
    resetFrameNode(frame);

    pageCache->frameCnt = pageCache->frameCnt - 1;

    return frame;
}

// get the frame from the page cache
Frame* searchPageFromCache(PageCache *const pageCache, int pageNum) {
    // get a frame based on page number
    int i;
    for(int i = 0; i < pageCache->capacity; i++) {
        Frame* frame = pageCache->arr[i];
        if(frame->pageNum == pageNum) {
            return frame;
        }
    }
    return NULL;
}

/* Statistics Interface */

// The getFrameContents function returns an array of PageNumbers (of size
// numPages). where the ith element is the number of the page stored in the ith
// page frame. An empty page frame is represented using the constant NO PAGE.
PageNumber *getFrameContents(BM_BufferPool *const bm) {
  if (bm == NULL) {
    return NULL;
  }

  int totalNumPages = bm->numPages;

  PageCache *pageCache = bm->mgmtData;

  PageNumber *arr = (PageNumber *) malloc(bm->numPages * sizeof(PageNumber));
  for (int i = 0; i < bm->numPages; i++) {
    arr[i] = pageCache->arr[i]->pageNum;
  }
  return arr;

}

// The getDirtyFlags function returns an array of bools (of size numPages) where
// the ith element. is TRUE if the page stored in the ith page frame is dirty.
// Empty page frames are considered as clean.
int *getDirtyFlags(BM_BufferPool *const bm) {
  if (bm == NULL) {
    return NULL;
  }

  PageCache *pageCache = bm->mgmtData;
  int numPages = bm->numPages;
  int *arr = (PageNumber *) malloc(numPages * sizeof(int));

  for (int i = 0; i < numPages; i++) {
    arr[i] = pageCache->arr[i]->dirtyBit;
  }
  return arr;
}

// The getFixCounts function returns an array of ints (of size numPages) where
// the ith element is the fix count of the page stored in the ith page frame.
// Return 0 for empty page frames.
int *getFixCounts(BM_BufferPool *const bm) {
  if (bm == NULL) {
    return NULL;
  }

  PageCache *pageCache = bm->mgmtData;
  int numPages = bm->numPages;
  int *arr = (PageNumber *) malloc(numPages * sizeof(int));

  for (int i = 0; i < numPages; i++) {
    arr[i] = pageCache->arr[i]->pinCount;
  }
  return arr;
}

//The function returns the number of pages that have been read from the disk since the buffer pool was initialized
int getNumReadIO(BM_BufferPool *const bm) {
  if (bm == NULL) {
    return -1;
  }
  // get page cache
  PageCache *pageCache = bm->mgmtData;
  return pageCache->numRead;
}

//The function returns the number of pages that have been written to the page file since the buffer pool was initialized
int getNumWriteIO(BM_BufferPool *const bm) {
  if (bm == NULL) {
    return -1;
  }
  // get page cache
  PageCache *pageCache = bm->mgmtData;
  return pageCache->numWrite;
}
