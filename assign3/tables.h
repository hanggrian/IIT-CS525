#ifndef TABLES_H
#define TABLES_H

#include "dt.h"

// Data Types, Records, and Schemas
typedef enum DataType {
	DT_INT = 0,
	DT_STRING = 1,
	DT_FLOAT = 2,
	DT_BOOL = 3
} DataType;

typedef struct Value {
	DataType dt;
	union v {
		int intV;
		char *stringV;
		float floatV;
		bool boolV;
	} v;
} Value;

typedef struct RID {
	int page;
	int slot;
} RID;

typedef struct Record
{
	RID id;
	char *data; // the binary representation of its attributes according to the schema
} Record;

// information of a table schema: its attributes, datatypes, 
typedef struct Schema
{
	int numAttr; // a number of attributes
	char **attrNames; // the name of each attribute
	DataType *dataTypes; // the data type of each attribute
	int *typeLength; // save the size of the strings if the attribute type is string
	int *keyAttrs; // an array of integers that are the positions of the attributes of the key
	int keySize; // the total number of keys
} Schema;

// TableData: Management Structure for a Record Manager to handle one relation
typedef struct RM_TableData
{
	char *name;
	Schema *schema;
	void *mgmtData;
} RM_TableData;

typedef struct PageDirectory {
    int pageNum; // the index of pageNum
    int count; // how many free slots are available in this page
    int firstFreeSlot; // the location of the first free slot in this page
	struct PageDirectory *pre;
	struct PageDirectory *next;
} PageDirectory; 

typedef struct PageDirectoryCache {
    int count;
    int capacity;
    PageDirectory *front;
    PageDirectory *rear;
}PageDirectoryCache;

typedef struct RecordNode {
	int page;
	int slot;
	char *data;
	struct RecordNode *pre;
	struct RecordNode *next;
}RecordNode;



#define MAKE_STRING_VALUE(result, value)				\
		do {									\
			(result) = (Value *) malloc(sizeof(Value));				\
			(result)->dt = DT_STRING;						\
			(result)->v.stringV = (char *) malloc(strlen(value) + 1);		\
			strcpy((result)->v.stringV, value);					\
		} while(0)


#define MAKE_VALUE(result, datatype, value)				\
		do {									\
			(result) = (Value *) malloc(sizeof(Value));				\
			(result)->dt = datatype;						\
			switch(datatype)							\
			{									\
			case DT_INT:							\
			(result)->v.intV = value;					\
			break;								\
			case DT_FLOAT:							\
			(result)->v.floatV = value;					\
			break;								\
			case DT_BOOL:							\
			(result)->v.boolV = value;					\
			break;								\
			}									\
		} while(0)


// debug and read methods
extern Value *stringToValue (char *value);
extern char *serializeTableInfo(RM_TableData *rel);
extern char *serializeTableContent(RM_TableData *rel);
extern char *serializeSchema(Schema *schema);
extern char *serializeRecord(Record *record, Schema *schema);
extern char *serializeAttr(Record *record, Schema *schema, int attrNum);
extern char *serializeValue(Value *val);

#endif
