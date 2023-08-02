#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"
#include "rm_serializer.h"


// implementations
char *
serializeTableInfo(RM_TableData *rel)
{
	VarString *result;
	MAKE_VARSTRING(result);

	APPEND(result, "TABLE <%s> with <%i> tuples:\n", rel->name, getNumTuples(rel));
	// append schema serialize data, to solve memory leak
	char *schemaSerilizeData = serializeSchema(rel->schema);
	APPEND_STRING(result, schemaSerilizeData);
	// to solve memory leak
	free(schemaSerilizeData);

	RETURN_STRING(result);
}

char * serializePageDirectories(PageDirectoryCache *pageDirectoryCache) 
{
	VarString *result;
	MAKE_VARSTRING(result);
	PageDirectory *p = pageDirectoryCache->front;
	while(p != NULL) {
		APPEND_STRING(result, serializePageDirectory(p));
		p = p->next;
	}
	RETURN_STRING(result);
}

char * serializePageDirectory(PageDirectory *pd)
{
	
	VarString *result;
	MAKE_VARSTRING(result);
    int attrSize = sizeof(int);
    char data[attrSize + 1];
    memset(data, '0', sizeof(char)*4);
 
	PageInfoToString(3, pd->pageNum, data);
	APPEND(result, "[%s-", data);

	memset(data, '0', sizeof(char)*4);
	PageInfoToString(3, pd->count, data);

	APPEND(result, "%s-", data);

	memset(data, '0', sizeof(char)*4);
	PageInfoToString(3, pd->firstFreeSlot, data);

	APPEND(result, "%s]", data);
	APPEND_STRING(result,"\n");
	RETURN_STRING(result);
}

PageDirectoryCache  *
deserializePageDirectories(char *pdStr)
{
	PageDirectory *front = NULL;
	PageDirectory *rear = NULL;
	char *token;
	token = strtok(pdStr, "\n");
	int count = 0;
	while(token != NULL) {
		PageDirectory *pd = (PageDirectory*)malloc(sizeof(PageDirectory));
		char data[5];
		memset(data, '0', sizeof(char)*4);
		strncpy(data, token + 1, 4);
		data[4] = '\0';
		pd->pageNum = (int)strtol(data, NULL, 10);
		memset(data, '0', sizeof(char)*4);
		strncpy(data, token + 6, 4);
		data[4] = '\0';
		pd->count = (int)strtol(data, NULL, 10);
		memset(data, '0', sizeof(char)*4);
		strncpy(data, token + 11, 4);
		data[4] = '\0';
		pd->firstFreeSlot = (int)strtol(data, NULL, 10);
		pd->next = NULL;
		pd->pre = NULL;
		if(front == NULL) {
			front = pd;
		} else {
			rear->next = pd;
			pd->pre = rear;
		}
		rear = pd;
		token = strtok(NULL, "\n");
		count = count + 1;
	}
	PageDirectoryCache *pageDirectoryCache = (PageDirectoryCache*)malloc(sizeof(PageDirectoryCache));
	pageDirectoryCache->front = front;
	pageDirectoryCache->rear = rear;
	pageDirectoryCache->count = count;
	return pageDirectoryCache;
}

char * 
serializeSchema(Schema *schema)
{
	int i;
	VarString *result;
	MAKE_VARSTRING(result);

	APPEND(result, "Schema with <%i> attributes (", schema->numAttr);

	for(i = 0; i < schema->numAttr; i++)
	{
		APPEND(result,"%s%s: ", (i != 0) ? ",": "", schema->attrNames[i]);
		switch (schema->dataTypes[i])
		{
		case DT_INT:
			APPEND_STRING(result, "INT");
			break;
		case DT_FLOAT:
			APPEND_STRING(result, "FLOAT");
			break;
		case DT_STRING:
			APPEND(result,"STRING[%i]", schema->typeLength[i]);
			break;
		case DT_BOOL:
			APPEND_STRING(result,"BOOL");
			break;
		}
	}
	APPEND_STRING(result,")");

	APPEND_STRING(result," with keys: {");

	for(i = 0; i < schema->keySize; i++)
		APPEND(result, "%s%s", ((i != 0) ? ",": ""), schema->attrNames[schema->keyAttrs[i]]);

	APPEND_STRING(result,"}\n");

	RETURN_STRING(result);
}

char * 
serializeTableContent(RM_TableData *rel)
{
	int i;
	VarString *result;
	RM_ScanHandle *sc = (RM_ScanHandle *) malloc(sizeof(RM_ScanHandle));
	Record *r = (Record *) malloc(sizeof(Record));
	MAKE_VARSTRING(result);

	for(i = 0; i < rel->schema->numAttr; i++)
		APPEND(result, "%s%s", (i != 0) ? ", " : "", rel->schema->attrNames[i]);

	startScan(rel, sc, NULL);

	while(next(sc, r) != RC_RM_NO_MORE_TUPLES)
	{
		APPEND_STRING(result,serializeRecord(r, rel->schema));
		APPEND_STRING(result,"\n");
	}
	closeScan(sc);

	RETURN_STRING(result);
}

char *
substring(const char *s, const char start, const char end)
{
    int i = 0, j = 0;
    while(s[i] != start) {
        i++;
    }
    i++;
    j = i;
    while(s[i] != end) {
        i++;
    }
    int size = i - j;
    char *temp = calloc(size + 1, sizeof(char));
    strncpy(temp, s + j,  size);
    temp[size] = '\0';
    return temp;
}

Schema * 
deserializeSchema(char *schemaData)
{
	Schema *schema = (Schema *) malloc(sizeof(Schema));

	char *numAttrStr = substring(schemaData, '<', '>');
	int numAttr = atoi(numAttrStr);
    schema->numAttr = numAttr;
	free(numAttrStr);
    
    // get attribute info
    char *attrInfo = substring(schemaData, '(', ')');
    parseAttrInfo(schema, attrInfo);
	free(attrInfo);

    // get keys info
    char *keyInfo = substring(schemaData, '{', '}');
    parseKeyInfo(schema, keyInfo);
	free(keyInfo);

	return schema;
}

void *
parseAttrInfo(Schema *schema, char *attrInfo) 
{
    char *t1;
    int numAttr = schema->numAttr;
    char **attrNames = (char **) malloc(sizeof(char*) * numAttr);
	DataType *dataTypes = (DataType *) malloc(sizeof(DataType) * numAttr);
	int *typeLength = (int *) malloc(sizeof(int) * numAttr);

    // char *s2 = "a: INT, b: STRING[4], c: INT";
	t1 = strtok(attrInfo, ": ");
    for(int i= 0; i < numAttr && t1 != NULL; ++i)
    {
        if(i != 0) {
            t1 = strtok (NULL,": ");
        } 
        int size = strlen(t1) + 1;
        attrNames[i] = (char *)malloc(size * sizeof(char));
        memcpy(attrNames[i], t1, size-1);
        attrNames[i][size-1] = '\0';

        // get data type
        t1 = strtok(NULL, ", ");

        // store string info
        int tokenSize = strlen(t1);

        if (t1[0] == 'I'){
            dataTypes[i] = DT_INT;
            typeLength[i] = 0;
        } else if (t1[0] == 'F'){
            dataTypes[i] = DT_FLOAT;
            typeLength[i] = 0;
        } else if (t1[0] == 'B'){
            dataTypes[i] = DT_BOOL;
            typeLength[i] = 0;
        } else if(t1[0] == 'S'){
            dataTypes[i] = DT_STRING;
        }
    }
	char *length = substring(attrInfo, '[', ']');
    int stringLength = atoi(length);
    schema->attrNames = attrNames;
    for(int i = 0; i < numAttr; i++) {
        if(dataTypes[i] == DT_STRING){
            typeLength[i] = stringLength;
        }
    }

    schema->dataTypes = dataTypes;
    schema->attrNames = attrNames;
    schema->typeLength = typeLength;
	free(length);
}

void *
parseKeyInfo(Schema *schema, char *keyInfo)
{
    
	int numAttr = schema->numAttr;
    int *keyAttrs = (int *) malloc(sizeof(int) * numAttr);

    // find the key index
    int index = -1;
    for(int i = 0; i < numAttr; i++) {
        if(strcmp(schema->attrNames[i], keyInfo) == 0) {
            index = i;
            break;
        }
    }
    if(index == -1) {
        printf("not valid attribute name\n");
    }
    for(int i= 0; i < numAttr; ++i)
    {
       keyAttrs[i] = index;
        
    }
    schema->keyAttrs = keyAttrs;
	schema->keySize = 1;
}

void PageInfoToString(int j,  int val,  char *data){
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

char * 
serializeRecord(Record *record, Schema *schema)
{
	VarString *result;
	MAKE_VARSTRING(result);
	int i;
    int attrSize = sizeof(int);
    char data[attrSize + 1];
    memset(data, '0', sizeof(char)*4);
 
	PageInfoToString(3, record->id.page, data);

	APPEND(result, "[%s", data);
	memset(data, '0', sizeof(char)*4);
	PageInfoToString(3, record->id.slot, data);
	APPEND(result, "-%s](", data);

	for(i = 0; i < schema->numAttr; i++)
	{
		APPEND_STRING(result, serializeAttr (record, schema, i));
		APPEND(result, "%s", (i == schema->numAttr - 1) ? "" : ",");
	}

	APPEND_STRING(result, ")\n");

	RETURN_STRING(result);
}

char * 
serializeAttr(Record *record, Schema *schema, int attrNum)
{
	int offset;
	char attrData[5];
	VarString *result;
	MAKE_VARSTRING(result);

	attrOffset(schema, attrNum, &offset);
	// attrData = record->data + offset;
	memcpy(attrData, record->data + offset, 4);
	attrData[4] = '\0';

	switch(schema->dataTypes[attrNum])
	{
	case DT_INT:
	{
		char val[5];
		memset(val, '\0', sizeof(int));
		memcpy(val, attrData, sizeof(int));
		APPEND(result, "%s:%s", schema->attrNames[attrNum], val);
	}
	break;
	case DT_STRING:
	{
		char *buf;
		int len = schema->typeLength[attrNum];
		buf = (char *) malloc(len + 1);
		strncpy(buf, attrData, len);
		buf[len] = '\0';

		APPEND(result, "%s:%s", schema->attrNames[attrNum], buf);
		free(buf);
	}
	break;
	case DT_FLOAT:
	{
		float val;
		memcpy(&val,attrData, sizeof(float));
		APPEND(result, "%s:%f", schema->attrNames[attrNum], val);
	}
	break;
	case DT_BOOL:
	{
		bool val;
		memcpy(&val,attrData, sizeof(bool));
		APPEND(result, "%s:%s", schema->attrNames[attrNum], val ? "TRUE" : "FALSE");
	}
	break;
	default:
		return "NO SERIALIZER FOR DATATYPE";
	}

	RETURN_STRING(result);
}

char *
serializeValue(Value *val)
{
	VarString *result;
	MAKE_VARSTRING(result);

	switch(val->dt)
	{
	case DT_INT:
		APPEND(result,"%i",val->v.intV);
		break;
	case DT_FLOAT:
		APPEND(result,"%f", val->v.floatV);
		break;
	case DT_STRING:
		APPEND(result,"%s", val->v.stringV);
		break;
	case DT_BOOL:
		APPEND_STRING(result, ((val->v.boolV) ? "true" : "false"));
		break;
	}
	RETURN_STRING(result);
}

Value *
stringToValue(char *val)
{
	Value *result = (Value *) malloc(sizeof(Value));

	switch(val[0])
	{
	case 'i':
		result->dt = DT_INT;
		result->v.intV = atoi(val + 1);
		break;
	case 'f':
		result->dt = DT_FLOAT;
		result->v.floatV = atof(val + 1);
		break;
	case 's':
		result->dt = DT_STRING;
		result->v.stringV = malloc(strlen(val));
		strcpy(result->v.stringV, val + 1);
		break;
	case 'b':
		result->dt = DT_BOOL;
		result->v.boolV = (val[1] == 't') ? TRUE : FALSE;
		break;
	default:
		result->dt = DT_INT;
		result->v.intV = -1;
		break;
	}

	return result;
}


RC 
attrOffset (Schema *schema, int attrNum, int *result)
{
	int offset = 0;
	int attrPos = 0;

	for(attrPos = 0; attrPos < attrNum; attrPos++) {
		switch (schema->dataTypes[attrPos])
		{
		case DT_STRING:
			offset += schema->typeLength[attrPos];
			break;
		case DT_INT:
			offset += sizeof(int);
			break;
		case DT_FLOAT:
			offset += sizeof(float);
			break;
		case DT_BOOL:
			offset += sizeof(bool);
			break;
		}
	}

	*result = offset;
	return RC_OK;
}

RecordNode *createRecordNode(int page, int slot, char *data, int sizeRecord) {
	RecordNode *node = (RecordNode *)malloc(sizeof(RecordNode));
	char *content = (char*)calloc(sizeRecord+1, sizeof(char));
	strcpy(content, data);
	content[strlen(content)] = '\0';
	node->page = page;
	node->slot = slot;
	node->data = data;
	node->pre = NULL;
	node->next = NULL;
	return node;
}

RecordNode *
deserializeRecords(Schema *schema, char *recordStr, int sizeRecord) 
{
	if(recordStr == NULL || schema == NULL) {
		return NULL;
	}
	char *token;
	char *a = strdup(recordStr);
    token = strtok(a, "\n");
	
	RecordNode *head = NULL;
	RecordNode *p = NULL;
	int i = 0;
    while(token != NULL) {
		Record *newRecord = (Record*)malloc(sizeof(Record));
		char *recordData = (char*)calloc(sizeRecord, sizeof(char));
		newRecord->data = recordData;
		char pageNum[5];
		memset(pageNum, '\0', 4);
		strncpy(pageNum, token + 1, 4);
		pageNum[4] = '\0';
		int page = (int)strtol(pageNum, NULL, 10);
		char slotNum[5];
		memset(slotNum, '\0', 4);
		strncpy(slotNum, token + 6, 4);
		slotNum[4] = '\0';
		int slot = (int)strtol(slotNum, NULL, 10);
		char data[13];
		memset(data, '\0', 12);
		strncpy(data, token + 14, 4);
		strncpy(data + 4, token + 21, 4);
		strncpy(data + 8, token + 28, 4);
		data[12] = '\0';
		char *temp = strdup(data);
		RecordNode *node = createRecordNode(page, slot, temp, sizeRecord);
		if(head == NULL) {
			head = node;
		} else {
			p->next = node;
		}
		p = node;
        token = strtok(NULL, "\n");
    }
	return head;
}

