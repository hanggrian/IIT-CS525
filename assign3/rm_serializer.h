#ifndef RM_SERIALIZER_H
#define RM_SERIALIZER_H

#include "dberror.h"
#include "tables.h"

/************************************************************
 *                    handle data structures                *
 ************************************************************/
// dynamic string
typedef struct VarString {
	char *buf;
	int size;
	int bufsize;
} VarString;

// convenience macros
#define MAKE_VARSTRING(var)				\
		do {							\
			var = (VarString *) malloc(sizeof(VarString));	\
			var->size = 0;					\
			var->bufsize = 100;					\
			var->buf = malloc(100);				\
		} while (0)

#define FREE_VARSTRING(var)			\
		do {						\
			free(var->buf);				\
			free(var);					\
		} while (0)

#define GET_STRING(result, var)			\
		do {						\
			result = malloc((var->size) + 1);		\
			memcpy(result, var->buf, var->size);	\
			result[var->size] = '\0';			\
		} while (0)

#define RETURN_STRING(var)			\
		do {						\
			char *resultStr;				\
			GET_STRING(resultStr, var);			\
			FREE_VARSTRING(var);			\
			return resultStr;				\
		} while (0)

#define ENSURE_SIZE(var,newsize)				\
		do {								\
			if (var->bufsize < newsize)					\
			{								\
				int newbufsize = var->bufsize;				\
				while((newbufsize *= 2) < newsize);			\
				var->buf = realloc(var->buf, newbufsize);			\
			}								\
		} while (0)

#define APPEND_STRING(var,string)					\
		do {									\
			ENSURE_SIZE(var, var->size + strlen(string));			\
			memcpy(var->buf + var->size, string, strlen(string));		\
			var->size += strlen(string);					\
		} while(0)

#define APPEND(var, ...)			\
		do {						\
			char *tmp = malloc(10000);			\
			sprintf(tmp, __VA_ARGS__);			\
			APPEND_STRING(var,tmp);			\
			free(tmp);					\
		} while(0)

/************************************************************
 *                    interface                             *
 ************************************************************/

// get the offset of the given attribute
extern RC attrOffset (Schema *schema, int attrNum, int *result);

// serialize data involved in the record manager
extern char * serializeTableInfo(RM_TableData *rel);
extern char * serializeTableContent(RM_TableData *rel);
extern char * serializeSchema(Schema *schema);
extern char * serializeRecord(Record *record, Schema *schema);
extern char * serializeAttr(Record *record, Schema *schema, int attrNum);
extern char * serializeValue(Value *val);
extern char * serializePageDirectory(PageDirectory *pd);
extern char * serializePageDirectories(PageDirectoryCache *tableCache);

// deserialize data involved in the record manager
extern void * deserializeTableInfo(RM_TableData *rel, char *tableInfo);
// extern RM_TableData * deserializeTableContent(char *tableContent);
extern Schema * deserializeSchema(char *schemaData);

extern PageDirectoryCache * deserializePageDirectories(char *pdStr);
extern RecordNode * deserializeRecords(Schema *schema, char *recordStr, int size);
extern Value * stringToValue(char *val);

// help interface
extern char * substring(const char *s, const char start, const char end);
extern void * parseAttrInfo(Schema *schema, char *attrInfo);
extern void * parseKeyInfo(Schema *schema, char *keyInfo);
extern PageDirectory * parsePageDirectory(char *t);
void parseRecord(Schema *schema, Record *record, char *token);
void PageInfoToString(int j,  int val,  char *data);

#endif