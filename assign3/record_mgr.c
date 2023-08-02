// This file implements all interfaces defined in record_mgr.c file

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tables.h"
#include "rm_serializer.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"


//stores scan data
typedef struct ScanCond{
    int currentPage;
    int currentSlot;
    Expr *condition;
} ScanCond;


// global variables
SM_FileHandle fHandle; // handle file operation
BM_BufferPool *bm; // handle buffer pool operation
BM_PageHandle *page; // handle buffer pool page operation
RecordNode *head = NULL;  // the header of the record nodes

int numTuples = 0; // the total number of tuples in this table
int sizeRecord; // the size of record
int capacity; // the max number of slots that can be used in a single page
int maxPageDiretories; // the max page directories that can be stored in a single page



// initialize a record manager
RC initRecordManager (void *mgmtData) 
{
    // printf("A record manager starts to work!\n");
    return RC_OK;
}

// shut down a record manager
RC shutdownRecordManager ()
{
    return RC_OK;
}


// creating a table is to create the underlying page file and store information
// about the scheme, free-space in the table information pages.
// here we set the name of table is the same as the file name
RC createTable (char *name, Schema *schema)
{
    // check validation of input parameters
    if(name == NULL || schema == NULL) {
        return RC_PARAMS_ERROR;
    }

    // check if the table specified by the name exists
    if(access(name, F_OK) == 0) {
        return RC_TABLE_EXISTS;
    } 

    // create a table with a given name
    if(createPageFile(name) != RC_OK) {
        return RC_TABLE_CREATES_FAILED;
    }

    // open this file
    if(openPageFile(name, &fHandle) != RC_OK) {
        return RC_ERROR;
    }

    // get serialize schema data
    char *schemaInfo = serializeSchema(schema);

    // write the schema data to page 0
    if(writeBlock(0, &fHandle, schemaInfo) != RC_OK) {
        free(schemaInfo);
        return RC_WRITE_FAILED;
    }

    // create a first empty page to store records
    PageDirectory *pd = createPageDirectoryNode(2);
    // store this page directory info to a reserved page(2)
    char *pdInfo = serializePageDirectory(pd);

    ensureCapacity(2, &fHandle);
    if(writeBlock(1, &fHandle, pdInfo) != RC_OK) {
        free(pdInfo);
        return RC_WRITE_FAILED;
    }

    // after page initialize, close those page to flush
    closePageFile(&fHandle);

    // initalize gloabl data
    numTuples = 0;

    // count the max slots that be used in a single page
    sizeRecord = getRecordSize(schema) + sizeof(int) + sizeof(int) + 2 + 2 + 2 + 3 + 1 + 3 + 1; 
    // set the capacity a little bit lower than the value PAGE_SIZE / sizeRecord to consider overhead
    capacity = 110; 
    
    // get max page directories that can be stored in a signle page
    maxPageDiretories = PAGE_SIZE / strlen(pdInfo);

    // release all resources
    free(schemaInfo);
    free(pd);
    free(pdInfo);
    return RC_OK;
}

// opening a table is to open a table since all operations require the table to be open first
// here we set the name of table is the same as the file name
RC openTable (RM_TableData *rel, char *name)
{
    // check validation of input parameters
    if(rel == NULL || name == NULL) {
        return RC_PARAMS_ERROR;
    }
    
    // check if the file specified by the filename exists
    if(access(name, F_OK) == -1) {
        return RC_TABLE_NOT_EXISTS;
    }

    // do preparations, initialize the buffer pool
    bm = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
    
    page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));

    initBufferPool(bm, name, 3, RS_FIFO, NULL);

    // read data from the page 0 since it stores table and schema info
    pinPage(bm, page, 0);

    // get schema info
    Schema *schema = deserializeSchema(page->data);
    unpinPage(bm, page);

    // read data from the page 1 since it stores all page directories info
    pinPage(bm, page, 1);

    // get all page directories 
    PageDirectoryCache *pageDirectoryCache = deserializePageDirectories(page->data); 

    // store filename
    rel->name = name;

    // store this schema
    rel->schema = schema;
   
    // store pageDirectoryCache
    rel->mgmtData = pageDirectoryCache;

    unpinPage(bm, page);
    
    return RC_OK;
}

// closing a table is to cause all outstanding changes to the table to be written to the page file
RC closeTable (RM_TableData *rel)
{
    // check the validation of rel
    if(rel == NULL) {
        return RC_PARAMS_ERROR;
    }

    // write all page directories info to page 1
    pinPage(bm, page, 1);
    PageCache *pageCache = bm->mgmtData;
    Frame *frame = searchPageFromCache(pageCache, page->pageNum);

    PageDirectoryCache *pageDirectoryCache = rel->mgmtData;
    char *pdInfo = serializePageDirectories(pageDirectoryCache);
    strcpy(frame->data, pdInfo);
    markDirty(bm, page);
    unpinPage(bm, page);
    forcePage(bm, page);

    // close the buffer pool
    shutdownBufferPool(bm);

    // release schema resource
    freeSchema(rel->schema);

    // release all pageDirectory nodes
    PageDirectory *p = pageDirectoryCache->front;
    while(p != NULL) {
        PageDirectory *temp = p->next;
        free(p);
        p = temp;
    }
    free(pageDirectoryCache);

    // free the page info
    free(page);
    

    return RC_OK;
}

// deleting a table is to delete this table
RC deleteTable (char *name)
{
    // check the validation of input parameters
    if(name == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    // check whether the file with given name exists
    if(access(name, F_OK) == -1) {
        return RC_TABLE_NOT_EXISTS;
    }
    return destroyPageFile(name);
}

// get the number of tuples in the table 
int getNumTuples (RM_TableData *rel)
{
    return numTuples;
}

RC flushDataToPage(char *data, int offset, int pageNum)
{
    if(data == NULL) {
        return RC_PARAMS_ERROR;
    }

    // find the frame to be written
    pinPage(bm, page, pageNum);
    PageCache* pageCache = bm->mgmtData;
    Frame* frame = searchPageFromCache(pageCache, page->pageNum);

    // copy this data to frame data
    memset(frame->data + offset, '\0', strlen(data));
    strcpy(frame->data + offset, data);

    markDirty(bm, page);
    unpinPage(bm, page);
    forcePage(bm, page);
    return RC_OK;
}

// handling records in a table
// get all records in current page
void getRecords(RM_TableData *rel, char *recordStr, int size)
{
    head = deserializeRecords(rel->schema, recordStr, size);
}


// insert a new record to the table
// when a new record is inserted, the record manager should assign an RID to 
// this record and update the record parameter passed to insertRecord
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

// update an existing record with new values
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



// retrieve a record with a certain RID
RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    // check validation of input parameters
    if(rel == NULL || record == NULL) {
        return RC_PARAMS_ERROR;
    }
  
    int recordPageNum = id.page;
    int recordSlot = id.slot;

    record->id.page = id.page;
    record->id.slot = id.slot;
    Schema *schema = rel->schema;

    pinPage(bm, page, recordPageNum);
    getRecords(rel, page->data, sizeRecord);
    RecordNode *p = head;
    while(p != NULL) {
        if(id.page == p->page && id.slot == p->slot) {
            record->data = strdup(p->data);
            break;
        }
        p = p->next;
    }
    if(record->data == NULL) {
        return RC_ERROR;
    }
    return RC_OK;
}

// scans: A client can initiate a scan to retrieve all tuples from a table
// that fulfill a certain condition.

// starting a scan initializes the RM_ScanHandle as an argument.
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    if(rel==NULL || scan == NULL || cond == NULL){
        return RC_PARAMS_ERROR;
    }

    scan->mgmtData=(ScanCond*)malloc(sizeof(ScanCond));
    if(scan->mgmtData==NULL){
        return RC_ALLOC_MEM_FAIL;
    }
    //Initialize scan data
    ScanCond *scanCond=(ScanCond*)scan->mgmtData;

    //records are stored starting from page 2 of file
    scanCond->currentPage=2; 
    scanCond->currentSlot=0;
    scanCond->condition=cond;

    scan->rel=rel;
    return RC_OK;
}

// return the next tuple that fulfills the scan condition.
// --if scan condition == NULL, then all tuples of the table should be returned.
// the function should return RC_RM_NO_MORE_TUPLES once the scan is completed 
// and RC_OK otherwise (unless an error occurs of course).
RC next (RM_ScanHandle *scan, Record *record)
{
    
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
        //if all slots have been scanned on current page, move to the next page
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

// closing a scan is to indicate the record manager that all associated resources can be cleaned up.
RC closeScan (RM_ScanHandle *scan)
{
    if(scan->mgmtData) {
        free(scan->mgmtData);
    }
    return RC_OK;
}

// dealing with schemas

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

// create a new schema
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, 
                        int *typeLength, int keySize, int *keys)
{
    // check validation of function parameters
    if(attrNames == NULL || dataTypes == NULL || typeLength == NULL || keys == NULL) {
        return NULL;
    }

    // allocate memory to a new schema
    Schema *schema = (Schema*)malloc(sizeof(Schema));

    if(schema == NULL) {
        // printf("the allocation memory to a schema is failed!\n");
        return NULL;
    }

    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keyAttrs = keys;
    schema->keySize = keySize;

    return schema;
}

// release all resources assigned to the given schema
RC freeSchema (Schema *schema)
{
    // check whether the schema is NULL
    if(schema == NULL) {
        return RC_OK;
    }

    // release all resources assigned to schema
    int numAttr = schema->numAttr;

    schema->numAttr = -1;
    if(schema->attrNames) {
        for(int i = 0; i < numAttr; i++) {
            free(schema->attrNames[i]);
        }
        free(schema->attrNames);
        schema->attrNames = NULL;
    }

    if(schema->dataTypes) {
        free(schema->dataTypes);
        schema->dataTypes = NULL;
    }

    if(schema->typeLength) {
        free(schema->typeLength);
        schema->typeLength = NULL;
    }

    if(schema->keyAttrs) {
        free(schema->keyAttrs);
        schema->keyAttrs = NULL;
    }

    free(schema);
    schema = NULL;

    return RC_OK;

}

// dealing with records and attribute values

// creating a record for a given scheme.
// Creating a new record should allocate enough memory to the data field to hold the 
// binary representations for all attributes of this record as determined by the schema.
RC createRecord (Record **record, Schema *schema)
{
    // check validations of function parameters
    if(record == NULL || schema == NULL) {
        return RC_PARAMS_ERROR;
    }
    // allocate memeory to store a new record
    Record *newRecord = (Record *)malloc(sizeof(Record));
    if(newRecord == NULL) {
        return RC_ALLOC_MEM_FAIL;
    }

    // initialize the page id and slot id
    newRecord->id.page = -1;
    newRecord->id.slot = -1;

    // allocate memory to store the binary representation
    int recordSize = getRecordSize(schema);
    char *recordData = (char*)calloc(recordSize, sizeof(char));
    newRecord->data = recordData;

    // pretend to add this record to the beginning of records
    *record = newRecord;

    return RC_OK;
}

// free all resources assigned to the given record
RC freeRecord (Record *record)
{
    // if the given record is NULL, do nothing
    if(record == NULL) {
        return RC_OK;
    }

    // the given record stored data, free these resources
    if(record->data) {
        free(record->data);
        record->data = NULL;
    }

    // free the given record
    free(record);
    record = NULL;

    return RC_OK;
}

// get string attribute value
RC getStringAttr(Record *record, Schema *schema, int attrNum, Value *attrValue, int offset) 
{
    // get string size
    int attrSize = sizeof(char) * schema->typeLength[attrNum];
    // allocate memory to store the record
    attrValue->v.stringV = (char*) malloc(attrSize + 1);
    // copy data from record to current position
    memcpy(attrValue->v.stringV, record->data + offset, attrSize);
    attrValue->v.stringV[attrSize] = '\0';
}


// get number attribute value
RC getNumAttr(Record *record, Schema *schema, int attrNum, Value *attrValue, int offset)
{

    int attrSize = 0;
    if(attrValue->dt == DT_INT) {
        attrSize = sizeof(int);
    } else if(attrValue->dt == DT_FLOAT) {
        attrSize = sizeof(float);
    } else if(attrValue->dt == DT_BOOL) {
        attrSize = sizeof(bool);
    }
    // allocate memory to store the record
    char *data = malloc(attrSize + 1);
    // copy data from record to current position
    memcpy(data, record->data + offset, attrSize);
    data[attrSize] = '\0';
    attrValue->v.intV = (int)strtol(data, NULL, 10);
    // release this resource
    free(data);
    data = NULL;
}

// get attribute values of a record
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    
    // check the validation of input parameters
    if(record == NULL || schema == NULL || value == NULL) {
        return RC_PARAMS_ERROR;
    }
 
    // get offset of attribute
    int offset = 0;
    if(attrOffset(schema, attrNum, &offset) != RC_OK) {
        printf("get attribute offset wrong\n");
        return RC_ERROR;
    }
    // store the record value
    Value *attrValue = (Value*) malloc(sizeof(Value));
    
    // get the attribute data type
    DataType dt = schema->dataTypes[attrNum];

    // store data type
    attrValue->dt = dt;

    // get attribut value based on data type
    if(dt == DT_STRING) {
        getStringAttr(record, schema, attrNum, attrValue, offset);
    } else if(dt == DT_INT || dt == DT_FLOAT || DT_BOOL) {
        getNumAttr(record, schema, attrNum, attrValue, offset);
    } else {
        return RC_DATATYPE_UNDEFINE;
    }

    // assign this attrValue to the given value in input parameters
    *value = attrValue;
    return RC_OK;
}


void intToString(int j,  int val,  char *data){
    int r = 0;
    int q = val;
    int last = j;
    while (q > 0 && j >= 0) {
        r = q % 10;
        q = q / 10;
        data[j] = data[j] + r;
        j--;
    }
    data[last+1] = '\0';
}

// set attribute values of a record
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    // check the validation of input parameters
    if(record == NULL || schema == NULL || value == NULL) {
        return RC_PARAMS_ERROR;
    }

    
    // a temporary variable to store the size of integer
    int attrSize = sizeof(int);

    // a temporary variable to store the record data
    char data[attrSize + 1];
    memset(data, '0', sizeof(char)*4);
 
    // get offset of attribute
    int offset = 0;
    if(attrOffset(schema, attrNum, &offset) != RC_OK) {
        printf("get attribute offset wrong\n");
        return RC_ERROR;
    }

    // check whether the given attrNum is the same data type as value
    if(value->dt != schema->dataTypes[attrNum]) {
        return RC_DATATYPE_MISMATCH;
    }
    
    // save the value to this record
    if(value->dt == DT_STRING) {
        // copy data from the given value to the record
        sprintf(record->data + offset, "%s", value->v.stringV);
    } else if(value->dt == DT_INT) {
        // change integer to string
        intToString(3, value->v.intV, data);
        sprintf(record->data + offset, "%s", data);
    } else if(value->dt == DT_FLOAT) {
        sprintf(record->data + offset,"%f" ,value->v.floatV);
    } else if(value->dt == DT_BOOL) {
        intToString(1, value->v.boolV, data);
        sprintf(record->data + offset,"%s" ,data);
    }

    return RC_OK;
}

PageDirectory *
createPageDirectoryNode(int pageNum) {
    PageDirectory *pd = (PageDirectory*)malloc(sizeof(PageDirectory));
    pd->pageNum = pageNum;
    pd->count = 0;
    pd->firstFreeSlot = 0;
    pd->pre = NULL;
    pd->next = NULL;
    return pd;
}

