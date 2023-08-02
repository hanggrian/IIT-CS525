# Buffer Manager

For this assignment, we implemented a simple buffer manager. The buffer manager acts as an intermediary between the dish and the requesting components. It allocates a portion of memory as a buffer pool to store requested data and adopts two replacement strategies to update its resources.

When the first request comes in, the buffer manager checks if the required data is already present in the buffer pool. If it is, the data is immediately retrieved from the buffer, avoiding the need for a disk access. This significantly speeds up the response time since memory access is much faster than disk access.

By implementing the buffer manager, the system achieves resource optimization in the following ways:

1. Memory utilization: The buffer manager efficiently allocates and manages the buffer pool, ensuring that memory is used optimally to store based on different strategies.  Additionally, the buffer manager tracks the usage of system resources and ensures that all allocated resources are properly released when the program terminates.
2. Reduced disk I/O: The buffer manager minimizes the number of disk reads by caching the storage management file handle in memory. This reduces the overall load on open and close files in disk.

## Sources

The following table shows all files in the `assign2` directory and explains their functionality.

| File                  | Description                                            |
| --------------------- | ------------------------------------------------------ |
| __buffer_mgr.*__      | Manages memory page frames and page files.             |
| __buffer_mgr_stat.*__ | Statistic interfaces of Buffer Manager.                |
| __dt.h__              | Boolean constants.                                     |
| store_mgr.*           | Responsible for managing database in files and memory. |
| dberror.*             | Keeps track and report different types of error.       |
| __test_assign2_1.c__  | Base test cases.                                       |
| __test_assign2_2.c__  | Extended test cases.                                   |
| test_helper.h         | Testing and assertion tools.                           |

## Execution Environment

Before running the whole project, please ensure that your environment has successfully installed `make` and `gcc`. Here is an example.

```
make --version
GNU Make 4.3
Built for x86_64-pc-linux-gnu
Copyright (C) 1988-2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
$ gcc --version
gcc (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
Copyright (C) 2021 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
```

## Compiling and Running

1. Enter into the folder containing all documents. `$ cd /assign2`

2. Execute `$ make`

3. See `test_assign2_1` results`$ ./test_assign2_1`

4. (Optional, make sure you already installed `valgrind` before execute this command)

   To check whether there are memory leaks:

    `valgrind --leak-check=full -v --show-leak-kinds=all ./test_assign2_1`

## Architectural Design

![Architecture screenshot.](https://github.com/hendraanggrian/IIT-CS525/raw/assets/assign2/architecture.png)

![Execution hierarchy.](https://github.com/hendraanggrian/IIT-CS525/raw/assets/assign2/hierarchy.png)

## Design Rationale

### 1. `initBufferPool`

Essentially, in the `initBufferPool` function, our objective is to create a new buffer pool consisting of page frames using a page replacement strategy. Specifically, we establish a `pageCache` to store these page frames in memory. The functionality of the `pageCache` varies depending on the selected strategy. For instance, if the replacement strategy is `FIFO`, the `pageCache` operates as a queue. On the other hand, if the strategy is LRU, it functions as an LRU cache, providing a hashMap-like functionality.

### 2. `shutdownBufferPool`

Essentially, the main function of  `shutdownBufferPool` is to release all resources assigned to the memory. To avoid memory leak, the buffer manager tracks the usage of system resources and ensures that all allocated resources are properly released when the program terminates.

### 3. `forceFlushPool`

The functionality of `forceFlushPool` is to force all dirty pages if exists from the buffer pool to be written to disk. One flush rule is that pin count of these dirty pages should be 0.

### 4. Buffer Manager Interface Access Pages

In this part, we implement the `pinPage`, `markDirty`, `unpinPage`, and `forcePage` functions. The mechanisms of `markDirty`, `unpinPage`, and `forcePage` are simple and similar. Here, we focus on explaining the mechanism of `pinPage` with different replacement policies.

#### 4.1 Workflow of `pinPage`

We implemented two kind of replacement strategy: `FIFO` and `LRU`. FIFO means first in first one while LRU means least recently used. The work flow is as following:

1. Initially, every frame is empty. We create an array to store those empty frames.

   ```c
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
   ```

   Store those empty frames:

   ```c
   pageCache->arr = (Frame**) malloc(numPages * sizeof(Frame*));
       int i;
       for(i = 0; i < pageCache->capacity; ++i ) {
           Frame* frame = createFrameNode();
           pageCache->arr[i] = frame;
       }
   ```

   ```c
   // initialize values for every attribute
   pageCache->front = 0;
   pageCache->rear = -1;
   pageCache->frameCnt = 0;
   pageCache->capacity = numPages;
   pageCache->numRead=0;
   pageCache->numWrite=0;
   ```

2. When a client pins a page:

   1) We check whether the required page exists in this page cache.

   ```C
   // check whether this pageNum hit the pageCache
   Frame* frame = isHitPageCache(pageCache, pageNum);
   ```

   2) If this frame exits, based on different replacement strategy, we update this frame information and LRU order.

   ```c
   // if yes, hit page cache
   if(frame != NULL) {
       page->pageNum = pageNum;
       page->data = frame->data;
       frame->pinCount++;
       if(bm->strategy == RS_LRU) {
       updateLRUOrder(pageCache, pageNum);
   }
   ```

   3) If this frame doesn't exist, we add this page to the page cache.

   ```c
   // if no execute different pin page processes based on replacement strategy
   if(bm->strategy == RS_FIFO) {
   	return addPageToPageCacheWithFIFO(bm, page, pageNum);
   } else if(bm->strategy == RS_LRU) {
   	return addPageToPageCacheWithLRU(bm, page, pageNum);
   }
   ```

   #### 4.2 Implementation of FIFO

   The main idea of FIFO here is by using a fixed-size circular array. Specifically, two variables `front` and `rear` are defined.  The`front` is used to track the first element of the page cache, while the `rear` to track the last elements of the queue.  The workflow of `addPageToPageCacheWithFIFO` is as following:

   - Check if the page cache is full.
     - If yes, remove the page based on FIFO.
   - For the first element, set value of `front` to 0.
   - Circularly increase the `rear` index by 1. If the rear reaches the end, next it would be at the start of the page cache.
   - Add the update frames in the position pointed to by `rear`.

   #### 4.2 Implementation of LRU

   The main idea of LRU here is by using a fixed-size hash array to store the page number in the page cache. Since the array is sorted, we use the index of this hash array to get the least recently used pages in the memory. The workflow of `addPageToPageCacheWithLRU` is as following:

   - Check if the page cache is full.
     - If yes:
       - Get the least used page number, which stored in `hash[0]`.
       - Mark the page cache is full and get this frame from the page cache.
     - If no, iterate all page number stored in hash and find the empty frame.
   - Update the frames.
   - Store the updated frames to the position get from the step 1.

## Testing Plan

1. We executed the `test_assign_2_1.c` and all testcase passed. (I just put part of result from screenshot)

   ```
   [test_assign2_1.c-Testing LRU page replacement-L269-14:47:11] OK: expected <[0 0],[-1 0],[-1 0],[-1 0],[-1 0]> and was <[0 0],[-1 0],[-1 0],[-1 0],[-1 0]>: check pool content reading in pages
   [test_assign2_1.c-Testing LRU page replacement-L269-14:47:11] OK: expected <[0 0],[1 0],[-1 0],[-1 0],[-1 0]> and was <[0 0],[1 0],[-1 0],[-1 0],[-1 0]>: check pool content reading in pages
   [test_assign2_1.c-Testing LRU page replacement-L269-14:47:11] OK: expected <[0 0],[1 0],[2 0],[-1 0],[-1 0]> and was <[0 0],[1 0],[2 0],[-1 0],[-1 0]>: check pool content reading in pages
   [test_assign2_1.c-Testing LRU page replacement-L269-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[-1 0]> and was <[0 0],[1 0],[2 0],[3 0],[-1 0]>: check pool content reading in pages
   [test_assign2_1.c-Testing LRU page replacement-L269-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content reading in pages
   [test_assign2_1.c-Testing LRU page replacement-L278-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L278-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L278-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L278-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L278-14:47:11] OK: expected <[0 0],[1 0],[2 0],[3 0],[4 0]> and was <[0 0],[1 0],[2 0],[3 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L288-14:47:11] OK: expected <[0 0],[1 0],[2 0],[5 0],[4 0]> and was <[0 0],[1 0],[2 0],[5 0],[4 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L288-14:47:11] OK: expected <[0 0],[1 0],[2 0],[5 0],[6 0]> and was <[0 0],[1 0],[2 0],[5 0],[6 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L288-14:47:11] OK: expected <[7 0],[1 0],[2 0],[5 0],[6 0]> and was <[7 0],[1 0],[2 0],[5 0],[6 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L288-14:47:11] OK: expected <[7 0],[1 0],[8 0],[5 0],[6 0]> and was <[7 0],[1 0],[8 0],[5 0],[6 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L288-14:47:11] OK: expected <[7 0],[9 0],[8 0],[5 0],[6 0]> and was <[7 0],[9 0],[8 0],[5 0],[6 0]>: check pool content using pages
   [test_assign2_1.c-Testing LRU page replacement-L293-14:47:11] OK: expected <0> and was <0>: check number of write I/Os
   [test_assign2_1.c-Testing LRU page replacement-L294-14:47:11] OK: expected <10> and was <10>: check number of read I/Os
   [test_assign2_1.c-Testing LRU page replacement-L301-14:47:11] OK: finished test
   ```

2. We also check whether there is memory leak through `valgrind`, and the result shows no errors.

```
==1357== LEAK SUMMARY:
==1357==    definitely lost: 0 bytes in 0 blocks
==1357==    indirectly lost: 0 bytes in 0 blocks
==1357==      possibly lost: 0 bytes in 0 blocks
==1357==    still reachable: 4,248 bytes in 9 blocks
==1357==         suppressed: 0 bytes in 0 blocks
==1357==
==1357== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## Contributing

The `assign1` had been done by **hwijaya@hawk.iit.edu (A20529195)**, **jlee252@hawk.iit.edu (A20324557)** and **xzhang143@hawk.iit.edu (A20494478)**. They work together to complete this task.
