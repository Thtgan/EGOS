#if !defined(__MEMORY_TEST_H)
#define __MEMORY_TEST_H

#if defined(CONFIG_UNIT_TEST_MM)
TEST_EXPOSE_GROUP(mm_testGroup);
#define UNIT_TEST_GROUP_MM  &mm_testGroup
#else
#define UNIT_TEST_GROUP_MM  NULL
#endif

#endif // __MEMORY_TEST_H
