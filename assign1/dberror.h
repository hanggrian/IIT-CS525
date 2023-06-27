#ifndef DBERROR_H
#define DBERROR_H

#define RC_OK 0

/* IO */
#define RC_FILE_NOT_FOUND 100
#define RC_READ_NON_EXISTING_PAGE 101

// Return code, referred to in `storage_mgr.h`.
typedef int RC;

#endif
