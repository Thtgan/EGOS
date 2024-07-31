#if !defined(__BIT_MACRO_H)
#define __BIT_MACRO_H

#include<kit/macro.h>
#include<kit/types.h>

#define ONE(__LENGTH)                                               (UINT(__LENGTH))1
#define FULL_MASK(__LENGTH)                                         (UINT(__LENGTH))-1
#define EMPTY_FLAGS                                                 0

#define LOGIC_EQUAL(__X, __Y)                                       (MACRO_EXPRESSION(__X) == MACRO_EXPRESSION(__Y))
#define LOGIC_NOT_EQUAL(__X, __Y)                                   (MACRO_EXPRESSION(__X) != MACRO_EXPRESSION(__Y))
#define LOGIC_AND(__X, __Y)                                         (MACRO_EXPRESSION(__X) && MACRO_EXPRESSION(__Y))
#define LOGIC_OR(__X, __Y)                                          (MACRO_EXPRESSION(__X) || MACRO_EXPRESSION(__Y))
#define LOGIC_NOT(__X)                                              (!MACRO_EXPRESSION(__X))

#define VAL_AND(__X, __Y)                                           (MACRO_EXPRESSION(__X) & MACRO_EXPRESSION(__Y))
#define VAL_OR(__X, __Y)                                            (MACRO_EXPRESSION(__X) | MACRO_EXPRESSION(__Y))
#define VAL_NOT(__X)                                                (~MACRO_EXPRESSION(__X))
#define VAL_XOR(__X, __Y)                                           (MACRO_EXPRESSION(__X) ^ MACRO_EXPRESSION(__Y))
#define VAL_LEFT_SHIFT(__X, __Y)                                    (MACRO_EXPRESSION(__X) << MACRO_EXPRESSION(__Y))
#define VAL_RIGHT_SHIFT(__X, __Y)                                   (MACRO_EXPRESSION(__X) >> MACRO_EXPRESSION(__Y))

#define FLAG(__FULL_LENGTH, __INDEX)                                VAL_LEFT_SHIFT(ONE(__FULL_LENGTH),  __INDEX)
#define FLAG8(__INDEX)                                              FLAG(8,     __INDEX)
#define FLAG16(__INDEX)                                             FLAG(16,    __INDEX)
#define FLAG32(__INDEX)                                             FLAG(32,    __INDEX)
#define FLAG64(__INDEX)                                             FLAG(64,    __INDEX)

#define SET_FLAG(__FLAGS, __FLAG)                                   VAL_OR(__FLAGS, __FLAG)
#define SET_FLAG_BACK(__FLAGS, __FLAG)                              (__FLAGS = SET_FLAG(__FLAGS, __FLAG))

#define CLEAR_FLAG(__FLAGS, __FLAG)                                 VAL_AND(__FLAGS, VAL_NOT(__FLAG))
#define CLEAR_FLAG_BACK(__FLAGS, __FLAG)                            (__FLAGS = CLEAR_FLAG(__FLAGS, __FLAG))

#define REVERSE_FLAG(__FLAGS, __FLAG)                               VAL_XOR(__FLAGS, __FLAG)
#define REVERSE_FLAG_BACK(__FLAGS, __FLAG)                          (__FLAGS = REVERSE_FLAG(__FLAGS, __FLAG))

#define TEST_VAL(__FLAGS, __MASK, __FLAG)                           LOGIC_EQUAL(VAL_AND(__FLAGS, __MASK), __FLAG)
#define TEST_FLAGS(__FLAGS, __FLAG)                                 LOGIC_EQUAL(VAL_AND(__FLAGS, __FLAG), __FLAG)
#define TEST_FLAGS_NONE(__FLAGS, __FLAG)                            LOGIC_EQUAL(VAL_AND(__FLAGS, __FLAG), 0)
#define TEST_FLAGS_FAIL(__FLAGS, __FLAG)                            LOGIC_NOT_EQUAL(VAL_AND(__FLAGS, __FLAG), __FLAG)
#define TEST_FLAGS_CONTAIN(__FLAGS, __FLAG)                         LOGIC_NOT_EQUAL(VAL_AND(__FLAGS, __FLAG), 0)

#define MASK_SIMPLE(__FULL_LENGTH, __LENGTH)                        VAL_RIGHT_SHIFT(FULL_MASK(__FULL_LENGTH), (__FULL_LENGTH) - (__LENGTH))
#define MASK_RANGE(__FULL_LENGTH, __BEGIN, __END)                   VAL_LEFT_SHIFT(MASK_SIMPLE(__FULL_LENGTH, (__END) - (__BEGIN)), __BEGIN)

#define TRIM_VAL(__VAL, __MASK)                                     VAL_AND(__VAL, __MASK)
#define TRIM_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)             TRIM_VAL(__VAL, MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define TRIM_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)        TRIM_VAL(__VAL, MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define FILL_VAL(__VAL, __MASK)                                     VAL_OR(__VAL, __MASK)
#define FILL_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)             FILL_VAL(__VAL, MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define FILL_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)        FILL_VAL(__VAL, MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define FLIP_VAL(__VAL, __MASK)                                     VAL_XOR(__VAL, __MASK)
#define FLIP_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)             FLIP_VAL(__VAL, MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define FLIP_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)        FLIP_VAL(__VAL, MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define CLEAR_VAL(__VAL, __MASK)                                    VAL_AND(__VAL, VAL_NOT(__MASK))
#define CLEAR_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)            CLEAR_VAL(__VAL, MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define CLEAR_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)       CLEAR_VAL(__VAL, MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define EXTRACT_VAL(__VAL, __FULL_LENGTH, __BEGIN, __END)           TRIM_VAL_SIMPLE(VAL_RIGHT_SHIFT(__VAL, __BEGIN), __FULL_LENGTH, (__END - __BEGIN))

#define LOWER_BIT(__VAL)                                            VAL_AND((__VAL), -(__VAL))

#endif // __BIT_MACRO_H
