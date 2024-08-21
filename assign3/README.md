# Record Manager

## Overview

For this assignment, we implemented a simple record manager. The main function
of the record manager is to handle tables with a fixed schema. Clients can
insert records, delete records, update records, and scan through the records in
a table. A scan is associated with a search condition and only returns records
that match the search condition. Each table will be stored in a separate page
file, and the record manager can access the pages of the file through the buffer
manager.

In our implementation, the record manager efficiently allocates and manages the
location of data pages in the database files and records the number of free
slots per page, ensuring that every page is used optimally to store records.
Additionally, the record manager tracks the usage of free space and ensures that
all allocated resources are properly released when the program terminates.

## Sources

The following table shows all files in the `assign3 directory and explains their
functionality.

File | Description
--- | ---
buffer_mgr.* | Manages memory page frames and page files.
buffer_mgr_stat.* | Statistic interfaces of Buffer Manager.
dt.h | Boolean constants.
__expr.*__ | Parse condition expression in the scan.
__record_mgr.*__ | Responsible for managing tables in this database.
store_mgr.* | Responsible for managing database in files and memory.
dberror.* | Keeps track and report different types of error.
__rm_serializer.*__ | Responsible for serialize and deserialize data stored in files.
__tables.h__ | Define useful data structures and functions to implement the record manager. |
__test_assign3_1.c__ | Base test cases.
__test_helper.h__ | Testing and assertion tools.
__test_expr.h__ | Testing the expression functions.

## Compiling and Running

1.  Enter into the folder containing all documents. `$ cd /assign3
1.  Execute `$ make all`
1.  See `test_assign3_1` results`$ ./test_assign3_1`

## Architectural Design

![Architecture screenshot.](https://github.com/hanggrian/IIT-CS525/raw/assets/assign3/architecture.png)

![Execution hierarchy.](https://github.com/hanggrian/IIT-CS525/raw/assets/assign3/hierarchy.svg)

## Design Rationale

Basically, the record manager will determine:

1.  How pages are organized in the file.
1.  How records are organized on each page.
1.  How each record is formatted.
1.  How to scan the record.

### Organize Pages in the file

We define a `PageDirectory` struct

```c
typedef struct PageDirectory {
    int pageNum; // the index of pageNum
    int count; // how many free slots are available in this page
    int firstFreeSlot; // the location of the first free slot in this page
    struct PageDirectory *pre;
    struct PageDirectory *next;
} PageDirectory;
```

The page directory acts as the header page and will be stored at one page. The
format of serialize data of page directory is `[0002-0004-0005]` while `0002`
represents `pageNum`, `0004` represents `count` and `0005` represents
`firstFreeSlot`. By doing this, we can get the `maxPageDirectories` that can be
stored in a single page.

```c
 maxPageDiretories = PAGE_SIZE / strlen(pdStr);
```

Additionally, we define a `PageDirectoryCache`struct to track all page
directories information.

```c
typedef struct PageDirectoryCache {
    int count;
    PageDirectory *front;
    PageDirectory *rear;
} PageDirectoryCache;
```

We create a new `PageDirectory` when the clients create the table.

```c
// create a first empty page to store records
PageDirectory *pd = createPageDirectoryNode(2);
// store this page directory info to a reserved page(2)
char *pdInfo = serializePageDirectory(pd);

ensureCapacity(2, &fHandle);
if(writeBlock(1, &fHandle, pdInfo) != RC_OK) {
    free(pdInfo);
    return RC_WRITE_FAILED;
}
```

We retrieve all page directories info when the clients open the  table.

```c
// read data from the page 1 since it stores all page directories info
pinPage(bm, page, 1);
// get all page directories
PageDirectoryCache *pageDirectoryCache = deserializePageDirectories(page->data);
```

Whenever the data pages become full, we allocate a new free data pages and the
tracker will move to the next free pages.

```c
// get the page directory info
PageDirectoryCache *pageDirectoryCache = rel->mgmtData;
int lastPageNum;
int lastFreeSlot;
int lastPageCount;
PageDirectory *lastPD;

PageDirectory *p = PageDirectoryCache->front;
// if the page exists an empty slot
while(p != NULL) {
    if(p->count < capacity) {
        lastPD = p;
        break;
    }
    p = p->next;
}
PageDirectory *pd;
// if all pages are full
if(p == NULL) {
    // get the last page directory and get its pageNum
    lastPageNum = pageDirectoryCache->rear->pageNum + 1;

    // create a new page directory node
    PageDirectory *pd = createPageDirectoryNode(lastPageNum);
    if(pageDirectoryCache->count % maxPageDiretories == 0)  {
        // this new page will store another page directories
        lastPageNum = pageDirectoryCache->rear->pageNum + 1;
        flushPageDirectoryToPage(pd, lastPageNum);
    }

    // update page directory cache
    pageDirectoryCache->rear->next = pd;
    pd->pre = pageDirectoryCache->rear;
    pageDirectoryCache->rear = pd;
    pageDirectoryCache->count = pageDirectoryCache->count + 1;

    // get last page directory
    lastPD = pageDirectoryCache->rear;
}
```

We release all page directory nodes when the clients close the table. Before we
release those resources, we need to make sure that the latest
`pageDirectoryCache` has been flushed to the file page.

```c
// release all pageDirectory nodes
PageDirectory *p = pageDirectoryCache->front;
while(p != NULL) {
    PageDirectory *temp = p->next;
    free(p);
    p = temp;
}
free(pageDirectoryCache);
```

### The Schema and Record

Record types are completely determined by the relation's schema. Here, we use
fixed-length records that only contain fixed-length fields. Flexible-length
records with the same schema consist of the same number of bytes.

When the clients create the table, we store this schema information on page 0.

```c
// get serialize schema data
char *schemaInfo = serializeSchema(schema);

// write the schema data to page 0
if(writeBlock(0, &fHandle, schemaInfo) != RC_OK) {
    free(schemaInfo);
    return RC_WRITE_FAILED;
}
```

When the clients open the table, we retrieve this schema information in page 0.

```c
 // read data from the page 0 since it stores table and schema info
 pinPage(bm, page, 0);

// get schema info
Schema *schema = deserializeSchema(page->data);
unpinPage(bm, page);

```

Since we used the fixed length records, we get the size of a record.

```c
// return the size in bytes of records for a given scheme
int getRecordSize (Schema *schema)
{
    // check the validation of schema
    if(schema == NULL) {
        return 0;
    }

    // get the size of a record
    int res = 0;
    DataType *dt = schema->dataTypes;
    int *typeLength = schema->typeLength;
    for(int i = 0; i < schema->numAttr; i++) {
        if (dt[i] == DT_INT) {
            res = res + sizeof(int);
        } else if(dt[i] == DT_STRING) {
            res = res + (sizeof(char) * typeLength[i]);
        } else if(dt[i] == DT_BOOL) {
            res = res + sizeof(bool);
        } else if(dt[i] == DT_FLOAT) {
            res = res + sizeof(float);
        }
    }
    return res;
}

```

We store record information in the file page in this format:

`[0002-0001](a:0002,b:bbbb,c:0002)`.

Here `[0002-0001]` represents the page number and slot, while
`(a:0002, b:bbbb, c:0002)` represents the record attributes and their values. We
use 4 bytes to store the integer values by defining converting functions. For
example, if `a = 1`, then we store `0001`, but if `a > 9999`, then we use
hexadecimal notation to store those data, ensuring that the integer value will
always occupy 4 bytes.

### how records are organized on each page

The records organization involves `createSchema`, `getRecord`, `insertRecord`,
`updateRecord`, and `deleteRecord`. The basic idea for every function is that we
create a `RecordNode` struct to store all records on the current page. The
`RecordNode` is a double-linked list. We iterate through all records to check
whether they are required. Once we find the target record, we can perform
`delete`, and `update` operations.

```c
typedef struct RecordNode {
	int page;
	int slot;
	char *data;
	struct RecordNode *pre;
	struct RecordNode *next;
} RecordNode;
```

When the client  executes`insert`, `update`, `delete`  commands, the
`RecordNode` will track those changes and do the related changes.

#### insert a record

```c
RC insertRecord (RM_TableData *rel, Record *record)
{
    // check the validation of input parameters
    if(rel == NULL || record == NULL) {
        return RC_PARAMS_ERROR;
    }

    // get the schema data in this table
    Schema *schema = rel->schema;

    // get the page directory info
    PageDirectoryCache *pageDirectoryCache = rel->mgmtData;

    int lastPageNum;
    int lastFreeSlot;
    int lastPageCount;
    PageDirectory *lastPD;

    // if the page exists an empty slot
    PageDirectory *p = pageDirectoryCache->front;
    while(p != NULL) {
        if(p->count < capacity) {
            lastPD = p;
            break;
        }
        p = p->next;
    }
    // if all pages are full
    if(p == NULL) {
        // get the last page directory and get its pageNum
        lastPageNum = pageDirectoryCache->rear->pageNum + 1;

        // create a new page directory node
        PageDirectory *pd = createPageDirectoryNode(lastPageNum);
        if(pageDirectoryCache->count % maxPageDiretories == 0)  {
            // this new page will store another page directories
            lastPageNum = pageDirectoryCache->rear->pageNum + 1;
            char *pdStr = serializePageDirectory(pd);
            flushDataToPage(pdStr, 0, lastPageNum);
        }

        // update page directory cache
        pageDirectoryCache->rear->next = pd;
        pd->pre = pageDirectoryCache->rear;
        pageDirectoryCache->rear = pd;
        pageDirectoryCache->count = pageDirectoryCache->count + 1;

        // get last page directory
        lastPD = pageDirectoryCache->rear;
    }

    if(lastPD == NULL) {
        return RC_ERROR;
    }

    lastPageNum = lastPD->pageNum;
    lastFreeSlot = lastPD->firstFreeSlot;
    lastPageCount = lastPD->count;

    // set page and slot to curretn record
    record->id.page = lastPageNum;
    record->id.slot = lastFreeSlot;

    // get current record offset
    int offset = record->id.slot * sizeRecord;

    // get this record serialization data
    char *recordStr = serializeRecord(record, schema);

    // store this record string to page
    flushDataToPage(recordStr, offset, lastPageNum);

    // update page directory cache
    lastPD->count = lastPD->count + 1;
    lastPD->firstFreeSlot = lastPD->firstFreeSlot + 1;
    rel->mgmtData = pageDirectoryCache;
    // update number of tuples
    numTuples++;

    return RC_OK;
}
```

#### update a record

```c
RC updateRecord (RM_TableData *rel, Record *record)
{
    // check the validation of input parameters
    if(rel == NULL || record == NULL) {
        return RC_PARAMS_ERROR;
    }

    // get page directories and schema information
    PageDirectoryCache *pageDirectoryCache = rel->mgmtData;
    PageDirectory *p = pageDirectoryCache->front;
    Schema *schema = rel->schema;

    // looking for this record
    while(p != NULL) {
        // find this record
        if(p->pageNum == record->id.page) {
            pinPage(bm, page, p->pageNum);

            Record *newRecord = (Record *)malloc(sizeof(Record));
            if(newRecord == NULL) {
                return RC_ALLOC_MEM_FAIL;
            }

            // initialize the page id and slot id
            newRecord->id.page = -1;
            newRecord->id.slot = -1;

            // allocate memory to store the binary representation
            char *recordData = (char*)calloc(sizeRecord, sizeof(char));
            newRecord->data = recordData;

            // create current record
            getRecord(rel, record->id, newRecord);
            newRecord->id.page = record->id.page;
            newRecord->id.slot = record->id.slot;
            newRecord->data = strdup(record->data);

            char *newRecordStr = serializeRecord(newRecord, schema);
            int offset = sizeRecord * newRecord->id.slot;
            PageCache* pageCache = bm->mgmtData;
            Frame* frame = searchPageFromCache(pageCache, page->pageNum);
            strcpy(frame->data + offset, newRecordStr);

            markDirty(bm, page);
            unpinPage(bm, page);
            forcePage(bm, page);
        }
        p = p->next;
    }
    return RC_OK;
}
```

#### delete a record

Deleting a record is a little bit different from `insert` and `update`. Here is
my idea: I find the target record and modify both its page number and slot to 0.
Then, I serialize this record and store it in its original place. After that,
when we update the `firstFreeSlot` in this page, I first get all records on this
page and find the first record with `pageNum = 0` and `slot = 0`. This way, we
can make full use of these free spaces. Here is the code.

```c
// delete a record with a certain RID
RC deleteRecord (RM_TableData *rel, RID id)
{
    if(rel == NULL) {
        return RC_PARAMS_ERROR;
    }
    PageDirectoryCache *pageDirectoryCache = rel->mgmtData;
    PageDirectory *p = pageDirectoryCache->front;
    Schema *schema = rel->schema;
    while(p != NULL) {
        if(p->pageNum == id.page) {

            pinPage(bm, page, p->pageNum);

            Record *newRecord = (Record *)malloc(sizeof(Record));
            if(newRecord == NULL) {
                return RC_ALLOC_MEM_FAIL;
            }

            // initialize the page id and slot id
            newRecord->id.page = -1;
            newRecord->id.slot = -1;

            // allocate memory to store the binary representation
            char *recordData = (char*)calloc(sizeRecord, sizeof(char));
            newRecord->data = recordData;

            // create current record
            getRecord(rel, id, newRecord);

            // update its page and slot to 0
            newRecord->id.page = 0;
            newRecord->id.slot = 0;

            char *recordStr = serializeRecord(newRecord, schema);

            // get this record offset
            int offset = sizeRecord * id.slot;

            // find the frame to be written
            pinPage(bm, page, p->pageNum);
            PageCache* pageCache = bm->mgmtData;
            Frame* frame = searchPageFromCache(pageCache, page->pageNum);
            strncpy(frame->data + offset, recordStr, sizeRecord);
            markDirty(bm, page);
            unpinPage(bm, page);
            forcePage(bm, page);

            // after that, get all records in this page
            getRecords(rel, page->data, sizeRecord);

            // update page directory cache
            p->count = p->count - 1;

            RecordNode *p1 = head;
            int i = 0;
            while(p1 != NULL) {
                if(p1->page == 0 && p1->slot == 0) {
                    p->firstFreeSlot = i;
                    break;
                }
                i++;
                p1++;

            }
            rel->mgmtData = pageDirectoryCache;
            numTuples--;
            break;
        }
        p = p->next;
    }
    return RC_OK;
}
```

### How to scan the record

We define a `ScanCond` to store the scan data

```c
typedef struct ScanCond{
    int currentPage;
    int currentSlot;
    Expr *condition;
} ScanCond;
```

The Scan function relies on `getRecord`. First, we set the first record's page
number and slot, retrieve that record from the file, and then parse this string
to get the required record. Here is the code.

```c
RC next (RM_ScanHandle *scan, Record *record)
{
    //record = (Record *)malloc(sizeof(Record));
    if(scan==NULL || record==NULL){
        return RC_ERROR;
    }

    RM_TableData *rel=(RM_TableData*)scan->rel;
    //gets scan condition from mgmtData
    ScanCond *scanCond=(ScanCond *)scan->mgmtData;
    //gets current page and slot
    int currentPage= scanCond->currentPage;
    int currentSlot=scanCond->currentSlot;

    PageDirectoryCache *pageDirectoryCache = rel->mgmtData;
    int maxPageNum = pageDirectoryCache->rear->pageNum;


    // Check if the current page and slot are within the valid range
    if (currentPage > maxPageNum || (currentPage <= maxPageNum && currentSlot >= capacity )) {
        // Unpin the current page before returning the RC_RM_NO_MORE_TUPLES
        return RC_RM_NO_MORE_TUPLES;
    }

    while(scanCond->currentPage<=maxPageNum){
        if(scanCond->currentSlot>=capacity){
            scanCond->currentSlot=0;
            scanCond->currentPage++;
            if(scanCond->currentPage % (maxPageDiretories + 1) == 0) {
                scanCond->currentPage++;
            }
            continue;
        }
        RID rid;
        rid.page=scanCond->currentPage;
        rid.slot=scanCond->currentSlot;
        getRecord(rel,rid,record);

        scanCond->currentSlot++;
        if(scanCond->condition==NULL){
            return RC_OK;
        }
        else{
            Value *result = (Value *) malloc(sizeof(Value));
            evalExpr(record,rel->schema,scanCond->condition, &result);
            if(result!=NULL && result->v.boolV!=0){
                freeVal(result);
                return RC_OK;
            }
            freeVal(result);
        }

    }
    scanCond->currentSlot=-1;

    return RC_RM_NO_MORE_TUPLES;
}
```

### Optional Extensions

For this assignment, we are implementing `TIDs and tombstones`. The basic idea
of tombstones is to use `MARK` in the map or old location to indicate that the
data in this current position has already been deleted. Whenever the client
deletes a record, we simply mark both the page number and slot occupied by this
deleted record as 0. Next, when we check whether there is a free slot to store
data in this page, we find this tombstone and insert the new data here. This
way, we make full use of the free space in the system

## Testing Result

Our implementations passed all testcases listed in `test_assign3_1` .

```shell
[test_assign3_1.c-test running muliple scans -L269-12:04:39] OK: expected true: scans returned same number of tuples
[test_assign3_1.c-test running muliple scans -L283-12:04:39] OK: finished test
```

## Interactive Interface

![Interactive screenshot.](https://raw.githubusercontent.com/hanggrian/IIT-CS525/assets/assign3/interactive.png)

Our interactive interface is a simple command-line app that organizes a student
database. The process is self-repeating until the user exists promptly or
forcibly.

## Contribution

The `assign3` had been done by **xzhang143@hawk.iit.edu (A20494478)**,
**jlee252@hawk.iit.edu (A20324557)** and **hwijaya@hawk.iit.edu (A20529195)**.
Specifically, Xue implemented most of the functions and fixed the issues raised
in the process; Jessica implemented the scan functions and conducted tests.
Hendra did a final test and tried to implement the `Interactive interface`
(optional extensions), but finally, we decided not to merge the
`Interactive interface` for the final `Makefile` in the submission.
