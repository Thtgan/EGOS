#if !defined(__ERROR_H)
#define __ERROR_H

#include<injector.h>
#include<kit/bit.h>
#include<kit/types.h>

#define BUILD_ERROR_CODE(__OBJECT, __STATUS)    (VAL_OR(VAL_LEFT_SHIFT(__OBJECT, 16), __STATUS))
#define ERROR_GET_OBJECT(__ERROR)               (EXTRACT_VAL(__ERROR, 32, 16, 32))
#define ERROR_GET_STATUS(__ERROR)               (EXTRACT_VAL(__ERROR, 32, 0, 16))

#define SET_ERROR_CODE(__OBJECT, __STATUS)      errorCode = BUILD_ERROR_CODE(__OBJECT, __STATUS)
#define GET_ERROR_CODE()                        errorCode

#define ERROR_OBJECT_EXECUTION                  0x0000
#define ERROR_OBJECT_MEMORY                     0x0001
#define ERROR_OBJECT_DEVICE                     0x0002
#define ERROR_OBJECT_FILE                       0x0003
#define ERROR_OBJECT_INDEX                      0x0004
#define ERROR_OBJECT_ITEM                       0x0005
#define ERROR_OBJECT_DATA                       0x0006
#define ERROR_OBJECT_ARGUMENT                   0x0007
#define ERROR_OBJECT_ADDRESS                    0x0008

#define ERROR_STATUS_NORMAL                     0x0000
#define ERROR_STATUS_NOT_FOUND                  0x0001
#define ERROR_STATUS_ALREADY_EXIST              0x0002
#define ERROR_STATUS_ACCESS_DENIED              0x0003
#define ERROR_STATUS_NO_FREE_SPACE              0x0004
#define ERROR_STATUS_OUT_OF_BOUND               0x0005
#define ERROR_STATUS_TIMEOUT                    0x0006
#define ERROR_STATUS_VERIFIVCATION_FAIL         0x0007
#define ERROR_STATUS_OPERATION_FAIL             0x0008

#endif // __ERROR_H
