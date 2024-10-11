#if !defined(__FCNTL_H)
#define __FCNTL_H

//FCNTL = File Control

#include<kit/types.h>

typedef Flags32 FCNTLopenFlags;

#include<kit/bit.h>

#define FCNTL_OPEN_READ_ONLY                    0x00000000
#define FCNTL_OPEN_WRITE_ONLY                   0x00000001
#define FCNTL_OPEN_READ_WRITE                   0x00000002
#define FCNTL_OPEN_EXTRACL_ACCESS_MODE(__FLAGS) TRIM_VAL_SIMPLE(__FLAGS, 32, 3) //Known as ACCMODE
//TODO: Not implemented
#define FCNTL_OPEN_CREAT                        FLAG32(8)   //Create if file not exist
//TODO: Not implemented
#define FCNTL_OPEN_EXCL                         FLAG32(9)   //Ensure that this call creates the file
//TODO: Not implemented
#define FCNTL_OPEN_NOCTTY                       FLAG32(10)  //If file is terminal device, process haver no control on it
//TODO: Not implemented
#define FCNTL_OPEN_TRUNC                        FLAG32(12)  //Truncate to length 0 if is file and allow writing
//TODO: Not implemented
#define FCNTL_OPEN_APPEND                       FLAG32(13)  //Data only apeend to end, ignoring seek
//TODO: Not implemented
#define FCNTL_OPEN_NONBLOCK                     FLAG32(14)  //Open file in nonblocking mode if possible
//TODO: Not implemented
#define FCNTL_OPEN_NDELAY                       FCNTL_OPEN_NONBLOCK
//TODO: Not implemented
#define FCNTL_OPEN_DSYNC                        FLAG32(16)  //Same as SYNC, for now
//TODO: Not implemented
#define FCNTL_OPEN_ASYNC                        FLAG32(17)  //Enable signal-driven I/O
//TODO: Not implemented
#define FCNTL_OPEN_DIRECT                       FLAG32(18)  //Try to minimize cache effects of the I/O to and from file
//TODO: Not implemented
#define FCNTL_OPEN_LARGEFILE                    FLAG32(20)  //WHen file size cannot be represented in 32 bit
//TODO: Not implemented
#define FCNTL_OPEN_DIRECTORY                    FLAG32(21)  //Fail if path is not a file
//TODO: Not implemented
#define FCNTL_OPEN_NOFOLLOW                     FLAG32(22)  //Fail if basename of path is symbolic link
//TODO: Not implemented
#define FCNTL_OPEN_NOATIME                      FLAG32(24)  //Do not update the file last access time
//TODO: Not implemented
#define FCNTL_OPEN_CLOEXEC                      FLAG32(25)  //Close file when exec(syscalls) returns, and test return value
//TODO: Not implemented
#define FCNTL_OPEN_SYNC                         FLAG32(26)  //Sychronize data to disk
//TODO: Not implemented
#define FCNTL_OPEN_PATH                         FLAG32(28)  //File has two purpose: indicate a location in the filesystem tree and to perform operations that act purely at the file descriptor level
//TODO: Not implemented
#define FCNTL_OPEN_EXEC                         FCNTL_OPEN_PATH
//TODO: Not implemented
#define FCNTL_OPEN_TMPFILE                      FLAG32(29)  //Create an unnamed temporary regular file (pathname for directory)

#define FCNTL_OPEN_DEFAULT_FLAGS                FCNTL_OPEN_READ_WRITE

#endif // __FCNTL_H
