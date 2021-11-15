#if !defined(__BIT_MACRO_H)
#define __BIT_MACRO_H

#include<kit/macro.h>
#include<types.h>

#define BIT_ONE(__LENGTH)                                           (UINT(__LENGTH))1
#define BIT_FULL_MASK(__LENGTH)                                     (UINT(__LENGTH))-1

#define BIT_LOGIC_EQUAL(__X, __Y)                                   (MACRO_EXPRESSION(__X) == MACRO_EXPRESSION(__Y))
#define BIT_LOGIC_NOT_EQUAL(__X, __Y)                               (MACRO_EXPRESSION(__X) != MACRO_EXPRESSION(__Y))
#define BIT_LOGIC_AND(__X, __Y)                                     (MACRO_EXPRESSION(__X) && MACRO_EXPRESSION(__Y))
#define BIT_LOGIC_OR(__X, __Y)                                      (MACRO_EXPRESSION(__X) || MACRO_EXPRESSION(__Y))
#define BIT_LOGIC_NOT(__X)                                          (!MACRO_EXPRESSION(__X))

#define BIT_AND(__X, __Y)                                           (MACRO_EXPRESSION(__X) & MACRO_EXPRESSION(__Y))
#define BIT_OR(__X, __Y)                                            (MACRO_EXPRESSION(__X) | MACRO_EXPRESSION(__Y))
#define BIT_NOT(__X)                                                (~MACRO_EXPRESSION(__X))
#define BIT_XOR(__X, __Y)                                           (MACRO_EXPRESSION(__X) ^ MACRO_EXPRESSION(__Y))
#define BIT_LEFT_SHIFT(__X, __Y)                                    (MACRO_EXPRESSION(__X) << MACRO_EXPRESSION(__Y))
#define BIT_RIGHT_SHIFT(__X, __Y)                                   (MACRO_EXPRESSION(__X) >> MACRO_EXPRESSION(__Y))

#define BIT_FLAG(__FULL_LENGTH, __INDEX)                            BIT_LEFT_SHIFT(BIT_ONE(__FULL_LENGTH),  __INDEX)
#define BIT_FLAG8(__INDEX)                                          BIT_FLAG(8,     __INDEX)
#define BIT_FLAG16(__INDEX)                                         BIT_FLAG(16,    __INDEX)
#define BIT_FLAG32(__INDEX)                                         BIT_FLAG(32,    __INDEX)
#define BIT_FLAG64(__INDEX)                                         BIT_FLAG(64,    __INDEX)

#define BIT_SET_FLAG(__FLAGS, __FLAG)                               BIT_OR(__FLAGS, __FLAG)
#define BIT_SET_FLAG_BACK(__FLAGS, __FLAG)                          (__FLAGS = BIT_SET_FLAG(__FLAGS, __FLAG))

#define BIT_CLEAR_FLAG(__FLAGS, __FLAG)                             BIT_AND(__FLAGS, BIT_NOT(__FLAG))
#define BIT_CLEAR_FLAG_BACK(__FLAGS, __FLAG)                        (__FLAGS = BIT_CLEAR_FLAG(__FLAGS, __FLAG))

#define BIT_REVERSE_FLAG(__FLAGS, __FLAG)                           BIT_XOR(__FLAGS, __FLAG)
#define BIT_REVERSE_FLAG_BACK(__FLAGS, __FLAG)                      (__FLAGS = BIT_REVERSE_FLAG(__FLAGS, __FLAG))

#define BIT_TEST_FLAGS(__FLAGS, __FLAG)                             BIT_LOGIC_EQUAL(BIT_AND(__FLAGS, __FLAG), __FLAG)
#define BIT_TEST_FLAGS_NONE(__FLAGS, __FLAG)                        BIT_LOGIC_EQUAL(BIT_AND(__FLAGS, __FLAG), 0)
#define BIT_TEST_FLAGS_FAIL(__FLAGS, __FLAG)                        BIT_LOGIC_NOT_EQUAL(BIT_AND(__FLAGS, __FLAG), __FLAG)
#define BIT_TEST_FLAGS_CONTAIN(__FLAGS, __FLAG)                     BIT_LOGIC_NOT_EQUAL(BIT_AND(__FLAGS, __FLAG), 0)

#define BIT_MASK_SIMPLE(__FULL_LENGTH, __LENGTH)                    BIT_RIGHT_SHIFT(BIT_FULL_MASK(__FULL_LENGTH), (__FULL_LENGTH) - (__LENGTH))
#define BIT_MASK_RANGE(__FULL_LENGTH, __BEGIN, __END)               BIT_LEFT_SHIFT(BIT_MASK_SIMPLE(__FULL_LENGTH, (__END) - (__BEGIN)), __BEGIN)

#define BIT_TRIM_VAL(__VAL, __MASK)                                 BIT_AND(__VAL, __MASK)
#define BIT_TRIM_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)         BIT_TRIM_VAL(__VAL, BIT_MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define BIT_TRIM_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)    BIT_TRIM_VAL(__VAL, BIT_MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define BIT_FILL_VAL(__VAL, __MASK)                                 BIT_OR(__VAL, __MASK)
#define BIT_FILL_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)         BIT_FILL_VAL(__VAL, BIT_MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define BIT_FILL_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)    BIT_FILL_VAL(__VAL, BIT_MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define BIT_FLIP_VAL(__VAL, __MASK)                                 BIT_XOR(__VAL, __MASK)
#define BIT_FLIP_VAL_SIMPLE(__VAL, __FULL_LENGTH, __LENGTH)         BIT_FLIP_VAL(__VAL, BIT_MASK_SIMPLE(__FULL_LENGTH, __LENGTH))
#define BIT_FLIP_VAL_RANGE(__VAL, __FULL_LENGTH, __BEGIN, __END)    BIT_FLIP_VAL(__VAL, BIT_MASK_RANGE(__FULL_LENGTH, __BEGIN, __END))

#define BIT_EXTRACT_VAL(__VAL, __FULL_LENGTH, __BEGIN, __END)       BIT_TRIM_VAL(BIT_RIGHT_SHIFT(__VAL, __BEGIN), BIT_MASK_SIMPLE(__FULL_LENGTH, (__END - __BEGIN)))

#endif // __BIT_MACRO_H
