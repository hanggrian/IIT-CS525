#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// Test name.
char *testName;

// Test output files.
#define TESTPF "test_pagefile.bin"

// Prototypes for test functions.
static void testCreateOpenClose(void);
static void testSinglePageContent(void);
static void testMultiplePageContent(void);
static void testExpandCapacity(void);

// Main function running all tests.
int main(void) {
  testName = "";

  initStorageManager();

  testCreateOpenClose();
  testSinglePageContent();
  testMultiplePageContent();
  testExpandCapacity();

  return 0;
}

// Try to create, open, and close a page file. Check a return code. If it is not
// RC_OK then output a message, error description, and exit.
void testCreateOpenClose(void) {
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile(TESTPF));

  TEST_CHECK(openPageFile(TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE(fh.totalNumPages == 1, "expect 1 page in new file");
  ASSERT_TRUE(fh.curPagePos == 0,
              "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE(openPageFile(TESTPF, &fh) != RC_OK,
              "opening non-existing file should return an error.");

  TEST_DONE();
}

// Test a single page content.
void testSinglePageContent(void) {
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));

  // create a new page file
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("created and opened file\n");

  // read first page into handle
  TEST_CHECK(readFirstBlock(&fh, ph));
  // the page should be empty (zero bytes)
  for (i = 0; i < PAGE_SIZE; i++) {
    ASSERT_TRUE(ph[i] == 0,
                "expected zero byte in first page of freshly initialized page");
  }
  printf("first block was empty\n");

  // change ph to be a string and write that one to disk
  for (i = 0; i < PAGE_SIZE; i++) {
    ph[i] = (i % 10) + '0';
  }
  printf("execute here in test===============\n");
  TEST_CHECK(writeBlock(0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock(&fh, ph));
  for (i = 0; i < PAGE_SIZE; i++) {
    ASSERT_TRUE(ph[i] == (i % 10) + '0',
                "character in page read from disk is the one we expected.");
  }
  printf("reading first block\n");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  TEST_DONE();
}

// Test multiple page content.
void testMultiplePageContent(void) {
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test Multiple page content";

  ph = (SM_PageHandle) calloc(PAGE_SIZE, sizeof(char));

  // create a new page file
  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));
  printf("created and opened file\n");

  // add new page
  TEST_CHECK(appendEmptyBlock(&fh));
  printf("Add new page with zero bytes\n");

  // read new page into handle
  TEST_CHECK(readNextBlock(&fh, ph));
  // the page should be empty (zero bytes)
  for (i = 0; i < PAGE_SIZE; i++) {
    ASSERT_TRUE(ph[i] == 0,
                "expected zero byte in new page of freshly initialized page");
  }
  printf("\n new block was empty\n");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  TEST_DONE();
}

// Try to increase capacity on a file by a certain number of pages.
void testExpandCapacity(void) {
  SM_FileHandle fh;

  testName = "test expandable capacity";

  TEST_CHECK(createPageFile(TESTPF));
  TEST_CHECK(openPageFile(TESTPF, &fh));

  ASSERT_TRUE(fh.totalNumPages == 1, "not expanded yet");
  TEST_CHECK(ensureCapacity(5, &fh));
  ASSERT_TRUE(fh.totalNumPages == 5, "expanded to 5");
  TEST_CHECK(ensureCapacity(10, &fh));
  ASSERT_TRUE(fh.totalNumPages == 10, "expanded again to 10");

  TEST_CHECK(closePageFile(&fh));
  TEST_CHECK(destroyPageFile(TESTPF));
  TEST_DONE();
}
