# Storage Manager

In this assignment, we implemented a simple storage manager that is capable of
reading blocks from a file on disk into memory and writing blocks from memory to
a file on disk. Basically, we implemented all interfaces described in
`storage_mgr.h`

## Files

File | Description
--- | ---
`store_mgr.*` | Responsible for managing database in files and memory.
`dberror.*` | Keeps track and report different types of error.
`test_assign1.c` | Contains main function running all tests.
`test_helper.h` | Testing and assertion tools.

## Execution Environment

Before running the whole project, please ensure that your environment has
successfully installed `make` and `gcc`. Here is an example.

```shell
make --version
GNU Make 4.3
Built for x86_64-pc-linux-gnu
Copyright (C) 1988-2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
```

```shell
$ gcc --version
gcc (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
Copyright (C) 2021 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
```

## Compiling and Running

1.  Enter into the folder containing all documents. `$ cd /assign1`
1.  Execute `$ make`
1.  See result s`$ ./test_assign1`

## Hierarchy

![Execution hierarchy.](https://github.com/hanggrian/IIT-CS525/raw/assets/assign1/hierarchy.svg)

## Implementation of interfaces

Here we describe the implementation of every interface listed in `storage mgr.h`

### 1. manipulating page files

```c
RC createPageFile(char *fileName)

{
	/* check the validation of fileName */
	if (fileName == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	/* creates the filename pointed to by filename */
	FILE *fp = fopen(fileName, "w+");

	/* checking if the file opening was unsuccessful */
	if (fp == NULL)
	{
		/* if yes, return an error showing file not found */
		return RC_FILE_NOT_FOUND;
	}

	/* allocates the PAGE_SIZE memory and assigns the value to str pointer */
	// char *str = (char*)malloc(PAGE_SIZE * sizeof(char));
	char *str = (char*)calloc(PAGE_SIZE, sizeof(char));

	/* writes data from the array pointer to, by str to the fp */
	fwrite(str, sizeof(char), PAGE_SIZE, fp);

	/* closes the file, and flushes buffers */
	fclose(fp);

	/* deallocates the memory allocated by the malloc */
	free(str);

	/* return ok to indicate all operations success in this function */
	return RC_OK;
}
```

```c
RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
	/* check the validation of fileName */
	if (fileName == NULL) {
		return RC_FILE_NOT_FOUND;
	}

    /* check the validation of fHandle*/
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* Opens the exsiting file */
	FILE *fp = fopen(fileName, "r+");

	/* checking if the file opening was unsuccessful*/
	if (fp == NULL) {
		/* if yes, return an error indicating that file not found */
		return RC_FILE_NOT_FOUND;
	}

	/* if the file was open successfully */

	/* store the file pointer to mgmtInfo */
	fHandle->mgmtInfo = fp;

	/* store the filename to file handle */
	fHandle->fileName = fileName;

	/* store current page position to the file handle */
	fHandle->curPagePos = 0;

	/* moves the file pointer to the end of the file */
	if (fseek(fp, 0, SEEK_END) != 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* calculate the file size of current opening file */
	long fileSize = ftell(fp);

	/* gets the totalNumPages and store this value to the file handle */
	fHandle->totalNumPages = (int)(fileSize / PAGE_SIZE);

	/* return ok to indicate all operations success in this function */
	return RC_OK;

}
```

```c
RC closePageFile (SM_FileHandle *fHandle) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* get the file pointer through fHandle */
	FILE *fp = fHandle->mgmtInfo;

	/* close the file, all buffers are flushed */
	fclose(fp);

	/* set fp to null */
	if (fp != NULL) {
		fp = NULL;
	}

	/* After the file closed and the file pointer sets to null, return ok */
	return RC_OK;

}
```

```c
RC destroyPageFile (char *fileName) {
	/* check the validation of fileName */
	if (fileName == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	/* call remove function to destroy the current page file */

	/* if the result is not zero, then some errors happened in this process, return errors information */
	if (remove(fileName) != 0) {
		return RC_FILE_NOT_FOUND;
	}

	/* the page file was removed successfully */
	return RC_OK;
}
```

### 2. reading blocks from disc

```c
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* check the validation of pageNum, since the pageNum starts with 0, so the valid range of pageNum should be range of totalNumPages */
	if (pageNum < 0 || pageNum >= fHandle->totalNumPages ) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* get the file pointer */
	FILE *fp = fHandle->mgmtInfo;

	/* if fp is null, return an error indicating that file not found */
	if (fp == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	/* get the offset of current pageNum to the beginning of this file */
	int offset = pageNum * PAGE_SIZE;

	/* check the current value of the position indicator, if not zero, errors happened in this process */
	if (fseek(fp, offset, SEEK_SET) != 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* before read data to memory page, check the validation of the memory page */
	if (memPage == NULL) {
		return RC_WRITE_FAILED;
	}

	/* reads data from the file to memory page */
	if (fread(memPage, sizeof(char), PAGE_SIZE, fp) < PAGE_SIZE) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* update current file page position */
	fHandle->curPagePos = pageNum;

	/* close the file */
	// fclose(fp);

	/* the read block operation was successfully */
	return RC_OK;

}
```

```c
int getBlockPos (SM_FileHandle *fHandle) {
	/* check the validatation of fHandle */
	if (fHandle == NULL) {
		return -1;
	}
	return fHandle->curPagePos;
}
```

```c
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* the first block number is page 0 */
	return readBlock(0, fHandle, memPage);
}
```

```c
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* get current block position */
	int pageNum = getBlockPos(fHandle);

	/* if the value of position is -1, errors happened in this process, return this error */
	if (pageNum == -1) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* call method of read block, the previous block should be pageNum - 1*/
	return readBlock(pageNum - 1, fHandle, memPage);
}
```

```c
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* get current block position */
	int pageNum = getBlockPos(fHandle);

	/* if the value of pageNum is -1, errors happened in this process, return these error */
	if (pageNum == -1) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* call method of read block, the next block should be pageNum + 1*/
	return readBlock(pageNum + 1, fHandle, memPage);
}
```

```c
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}
	/* get the last block pageNum */
	int lastBlockPageNum = fHandle->totalNumPages - 1;

	/* call method of read block, the previous block should be pageNum - 1*/
	return readBlock(lastBlockPageNum, fHandle, memPage);
}
```

```c
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* get current block position */
	int pageNum = getBlockPos(fHandle);

	/* if the value of pageNum is -1, errors happened in this process, return these error */
	if (pageNum == -1) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* call method of read block */
	return readBlock(pageNum, fHandle, memPage);

}
```

### 3. writing blocks to a page file

```c
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* check the validation of fHandle*/
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* check the validation of pageNum */
	if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* get the current file pointer */
	// FILE *fp = fopen(fHandle->fileName, "w+");
	FILE *fp = fHandle->mgmtInfo;

	/* if the file pointer not found, return file not found */
	if (fp == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	/* the file was found successfully */
	/* move the current fp to the current page of the file */
	int offset = pageNum * PAGE_SIZE;

	if(fseek(fp, offset, SEEK_SET) != 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* before fwrite data from memory to disk, check the validation of memPage */
	if (memPage == NULL) {
		return RC_WRITE_FAILED;
	}

	/* write data from memory to dish */
	if(fwrite(memPage, sizeof(char), strlen(memPage), fp) < PAGE_SIZE) {
		return RC_WRITE_FAILED;
	}

	/* update current page position */
	fHandle->curPagePos = pageNum;
	return RC_OK;
}
```

```c
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* get current page number */
	int curPageNum = fHandle->curPagePos;

	/* write block */
	return writeBlock(curPageNum, fHandle, memPage);
}
```

```c
RC appendEmptyBlock (SM_FileHandle *fHandle) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* creates the filename pointed to by filename */
	FILE *fp = fHandle->mgmtInfo;

	/* checking if the file opening was unsuccessful */
	if (fp == NULL)
	{
		/* if yes, return an error showing file not found */
		return RC_FILE_NOT_FOUND;
	}

	/* move the file pointer to the end of the file */
	if (fseek(fp, 0, SEEK_END) != 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* after the file point moves to the end of the file, append empty block to this file*/

	/* allocates the PAGE_SIZE memory and assigns the value to str pointer */
	char *str = (char*)calloc(PAGE_SIZE, sizeof(char));

	/* writes data from the array pointer to, by str to the fp */
	if (fwrite(str, sizeof(char), PAGE_SIZE, fp) < PAGE_SIZE) {
		free(str);
		return RC_WRITE_FAILED;
	}

	/* if write succesfully */
	/* plus 1 to total number of pages */
	fHandle->totalNumPages++;

	/* closes the file, and flushes buffers */
	// fclose(fp);

	/* deallocates the memory allocated by the malloc */
	free(str);

	/* return ok to indicate all operations success in this function */
	return RC_OK;
}
```

```c
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
	/* check the validation of fHandle */
	if (fHandle == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}

	/* check the validation of number of pages */
	if (numberOfPages < 1) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	/* get the current total number of pages */
	int currentNumPages = fHandle->totalNumPages;

	/* get how many pages to be added by comparing number of pages and current number of pages */
	int cnt = numberOfPages - currentNumPages;

	/* if the pages to be added is greater to zero */
	if (cnt > 0) {
		for(int i = 0; i < cnt; i++) {
			appendEmptyBlock(fHandle);
		}
	}

	/* after executed appendEmptyBlock, check whether the total number of pages is the same as required number of pages */
	if (fHandle->totalNumPages != numberOfPages) {
		return RC_WRITE_FAILED;
	}

	/* return ok to indicate all operations success in this function */
	return RC_OK;
}
```

## Extra Testcases

We had added two testcases `static void testMultiplePageContent(void)`
and `static void testExpandCapacity(void)` to test what happened when there are
multiple page and when the system need to expand capacity.

```c
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
```

```c
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
```

## Contributing

The `assign1` had been done by hwijaya@hawk.iit.edu (A20529195),
jlee252@hawk.iit.edu (A20324557) and

xzhang143@hawk.iit.edu (A20494478). They work together to complete this task.
