#if !defined(__RETURN_VALUE_H)
#define __RETURN_VALUE_H

#include<kit/bit.h>
#include<kit/types.h>

typedef int ReturnValue;

#define RETURN_VALUE_RETURN_NORMALLY                    0x00000000
#define RETURN_VALUE_GENERAL_ERROR                      0xFFFFFFFF
#define RETURN_VALUE_ERROR_FLAG                         0x80000000

#define BUILD_ERROR_RETURN_VALUE(__OBJECT, __STATUS)    VAL_OR(RETURN_VALUE_ERROR_FLAG, (VAL_OR(VAL_LEFT_SHIFT(__OBJECT, 16), __STATUS)))
#define RETURN_VALUE_IS_ERROR(__RETURN_VALUE)           (TEST_FLAGS(__RETURN_VALUE, RETURN_VALUE_ERROR_FLAG))
#define RETURN_VALUE_GET_OBJECT(__RETURN_VALUE)         (EXTRACT_VAL(__RETURN_VALUE, 32, 16, 32))
#define RETURN_VALUE_GET_STATUS(__RETURN_VALUE)         (EXTRACT_VAL(__RETURN_VALUE, 32, 0, 16))

#define RETURN_VALUE_OBJECT_EXECUTION                   0x0000
#define RETURN_VALUE_OBJECT_MEMORY                      0x0001
#define RETURN_VALUE_OBJECT_DEVICE                      0x0002
#define RETURN_VALUE_OBJECT_FILE                        0x0003
#define RETURN_VALUE_OBJECT_INDEX                       0x0004
#define RETURN_VALUE_OBJECT_ITEM                        0x0005
#define RETURN_VALUE_OBJECT_DATA                        0x0006
#define RETURN_VALUE_OBJECT_ARGUMENT                    0x0007
#define RETURN_VALUE_OBJECT_ADDRESS                     0x0008

#define RETURN_VALUE_STATUS_NORMAL                      0x0000
#define RETURN_VALUE_STATUS_NOT_FOUND                   0x0001
#define RETURN_VALUE_STATUS_ALREADY_EXIST               0x0002
#define RETURN_VALUE_STATUS_ACCESS_DENIED               0x0003
#define RETURN_VALUE_STATUS_NO_FREE_SPACE               0x0004
#define RETURN_VALUE_STATUS_OUT_OF_BOUND                0x0005
#define RETURN_VALUE_STATUS_TIMEOUT                     0x0006
#define RETURN_VALUE_STATUS_VERIFIVCATION_FAIL          0x0007
#define RETURN_VALUE_STATUS_OPERATION_FAIL              0x0008

#endif // __RETURN_VALUE_H
