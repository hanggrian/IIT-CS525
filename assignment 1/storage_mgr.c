/*reserve for team member information */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "dberror.h"

/* This file defines the implementation of interfaces provided in storage_mgr.h */


/**
 * Instantiate the storage manager by printing a messessge to standard out
*/
void initStorageManager(void) {
	printf("The program begins to initialize storage manager.\n ");
}

/**
 * Define implementation of createPageFile. 
 * The createPageFile function is to create a new page file with one page size.
 * This page file fills with '\0' bytes.
*/
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

/**
 * Define implementation of openPageFile. 
 * The openPageFile function is to open an existing file and get statistic data and store those to the file handle.
 * If the file doesn't exist, return RC_FILE_NOT_FOUND.
 * If opening the file is successful, then the files of this file handle is initialized with the information about the opened file.
 * For example, the information about the opened file may contain file name, total number of pages, current page position 
*/
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


/**
 * Define implementation of closePageFile. 
 * The closePageFile method is to close the current page file.
*/
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


/**
 * Define implementation of destroyPageFile. 
 * The destroyPageFile method is to delete the page file based on filename.
*/
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

/* reading blocks from disc */

/**
 * Define implementation of readBlock. 
 * The readBlock method is to read the pageNum block from a file and stores its content in the memory pointed 
 * to by the memPage page handle.
 * If the file has less than pageNum pages, the method should return RC READ NON EXISTING PAGE.
*/
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


/**
 * Define implementation of getBlockPos. 
 * The getBlockPos method is to get the current page position in a file. 
*/
int getBlockPos (SM_FileHandle *fHandle) {
	/* check the validatation of fHandle */
	if (fHandle == NULL) {
		return -1;
	}
	return fHandle->curPagePos;
}

/**
 * Define implementation of readFirstBlock. 
 * The readFirstBlock method is to read the first page in a file. 
 * The current page position should be moved to the page that was read.
 * If the user tries to read a block before the first page or after the last page of the file, 
 * the method should return RC READ NON EXISTING PAGE.
*/
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	/* the first block number is page 0 */
	return readBlock(0, fHandle, memPage);
}

/**
 * Define implementation of readPreviousBlock. 
 * The readPreviousBlock method is to read the previous page relative to the current page position of the file. 
 * The current page position should be moved to the page that was read.
 * If the user tries to read a block before the first page or after the last page of the file, 
 * the method should return RC READ NON EXISTING PAGE.
*/
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

/**
 * Define implementation of readCurrentBlock. 
 * The readCurrentBlock method is to read the current page position of the file. 
 * The current page position should be moved to the page that was read.
 * If the user tries to read a block before the first page or after the last page of the file, 
 * the method should return RC READ NON EXISTING PAGE.
*/
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

/**
 * Define implementation of readNextBlock. 
 * The readNextBlock method is to read the next page relative to the current page position of the file. 
 * The current page position should be moved to the page that was read.
 * If the user tries to read a block before the first page or after the last page of the file, 
 * the method should return RC READ NON EXISTING PAGE.
*/
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

/**
 * Define implementation of readLastBlock. 
 * The readLastBlock method is to read the last page in a file. 
*/
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


/* writing blocks to a page file */

/**
 * Define implementation of writeBlock. 
 * The writeBlock method is to write a page date to disk using either the current position or an absolute position. 
*/
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

	/* close fp to flush */
	// fclose(fp);

	return RC_OK;

}

/**
 * Define implementation of writeCurrentBlock. 
 * The writeCurrentBlock method is to write current page to disk using either the current position or an absolute position. 
*/
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

/**
 * Define implementation of appendEmptyBlock. 
 * The appendEmptyBlock method is to increase the number of pages in the file by one.
 * The new last page should be filled with zero bytes.
*/
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

/**
 * Define implementation of ensureCapacity. 
 * The ensureCapacity method is to check current capacity.
 * If the file has less than number of pages, then increase the size to the number of pages.
*/
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
