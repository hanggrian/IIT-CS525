#include <stdlib.h>

#include "dberror.h"

#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"

#define ID_LENGTH 0
#define NAME_LENGTH 10

typedef struct StudentRecord {
  int id;
  char *name;
} StudentRecord;

Record *asRecord(Schema *schema, StudentRecord record);
void interactiveCreate(char *fileName);
void interactiveView(void);
void interactiveInsert(int studentId, char *studentName);
void interactiveUpdate(int studentId, char *studentName);
void interactiveDelete(int studentId);

Schema *interactiveSchema;
RM_TableData *interactiveTable;

void recursive() {
  printf("\n1. Create table\n"
         "2. View table\n"
         "3. Insert student\n"
         "4. Update student name\n"
         "5. Delete student\n"
         "\n"
         "V. View\n"
         "E. Exit\n"
         "\n"
         "What would you like to do:\n");
  char root[1];
  scanf("%s", root);

  if (strcmp(root, "1") == 0) {
    printf("\nEnter table name:\n");
    char tableName[30];
    scanf("%s", tableName);

    interactiveCreate(tableName);
  } else if (strcmp(root, "2") == 0) {
    interactiveView();
  } else if (strcmp(root, "3") == 0) {
    printf("\nNew student ID:\n");
    int studentId;
    scanf("%d", &studentId);

    printf("\nNew student name:\n");
    char studentName[NAME_LENGTH];
    scanf("%s", studentName);

    interactiveInsert(studentId, studentName);
  } else if (strcmp(root, "4") == 0) {
    printf("\nExisting student ID:\n");
    int studentId;
    scanf("%d", &studentId);

    printf("\nChange student name:\n");
    char studentName[NAME_LENGTH];
    scanf("%s", studentName);

    interactiveUpdate(studentId, studentName);
  } else if (strcmp(root, "5") == 0) {
    printf("\nExisting student ID:\n");
    int studentId;
    scanf("%d", &studentId);

    interactiveDelete(studentId);
  } else if (strcmp(root, "e") == 0 || strcmp(root, "E") == 0) {
    printf("\nGoodbye!\n");
    closeTable(interactiveTable);
    exit(0);
  } else {
    printf("Unknown input!\n");
    closeTable(interactiveTable);
    exit(1);
  }

  recursive();
}

int main() {
  printf("\nSTUDENTS DATABASE\n");
  recursive();
  return 0;
}

void interactiveCreate(char *fileName) {
  char *names[] = {"id", "name"};
  DataType types[] = {DT_INT, DT_STRING};
  int sizes[] = {ID_LENGTH, NAME_LENGTH};
  int keys[] = {0};

  interactiveSchema = createSchema(2, names, types, sizes, 1, keys);
  createTable(fileName, interactiveSchema);

  interactiveTable = (RM_TableData *) malloc(sizeof(RM_TableData));
  openTable(interactiveTable, fileName);

  printf("Table created!\n");
}

void interactiveView(void) {

}

void interactiveInsert(int studentId, char *studentName) {
  StudentRecord record = {studentId, studentName};
  insertRecord(interactiveTable, asRecord(interactiveSchema, record));

  printf("Tuple inserted!\n");
}

void interactiveUpdate(int studentId, char *studentName) {
  StudentRecord record = {studentId, studentName};
  updateRecord(interactiveTable, asRecord(interactiveSchema, record));

  printf("Tuple updated!\n");
}

void interactiveDelete(int studentId) {
  RID rid;
  rid.page = studentId;
  deleteRecord(interactiveTable, rid);

  printf("Tuple deleted!\n");
}

Record *asRecord(Schema *schema, StudentRecord record) {
  Record *result;
  Value *value;
  createRecord(&result, schema);

  MAKE_VALUE(value, DT_INT, record.id);
  freeVal(value);

  MAKE_STRING_VALUE(value, record.name);
  freeVal(value);

  return result;
}
