#if !defined(__KERNEL_STRING_H)
#define __KERNEL_STRING_H

#include<stddef.h>

/**
 * @brief Return the length of the string, ends with '\0'
 * 
 * @param str String
 * @return size_t The length of the string
 */
size_t strlen(const char* str);

/**
 * @brief Return the first position in str1 where character not appeared in str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return size_t First position in str1 where character not appeared in str2, length of str1 if all characters in str1 contained in str2
 */
size_t strspn(const char* str1, const char* str2);

/**
 * @brief Return the first position in str1 where character appeared in str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return size_t First position in str1 where character appeared in str2, length of str1 if all characters in str1 not contained in str2
 */
size_t strcspn(const char* str1, const char* str2);

/**
 * @brief Return the pointer to the position str2 first appeared in str1
 * 
 * @param str1 String
 * @param str2 Pattern to search
 * @return char* Position str2 first appeared in str1, NULL if str2 not appeared in str1
 */
char* strstr(const char* str1, const char* str2);

/**
 * @brief Split string into tokens, '\0' will be added to end of the token
 * 
 * @param str String to truncate, if NULL, continue truncating previous string
 * @param delimiters Delimeters in str
 * @return char* The beginning of the token, NULL if token not exists
 */
char* strtok(char* str, const char* delimiters);

/**
 * @brief Return the pointer to the first character both appeared in str1 and str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return char* Pointer to the first character both appeared in str1 and str2, NULL if all characters in str1 not appeared in str2
 */
char* strpbrk(const char* str1, const char* str2);

/**
 * @brief Return the first position where character appear in string
 * 
 * @param str String
 * @param ch Character to search
 * @return char* First position where character appear in string, NULL if character not appeared in string
 */
char* strchr(const char* str, int ch);

/**
 * @brief Return the last position where character appear in string
 * 
 * @param str String
 * @param ch Character to search
 * @return char* Last position where character appear in string, NULL if character not appeared in string
 */
char* strrchr(const char* str, int ch);

/**
 * @brief Copy string
 * 
 * @param des Copy destination
 * @param src Copy source
 * @return char* des
 */
char* strcpy(char* des, const char* src);

/**
 * @brief Copy string with limitation
 * 
 * @param des Copy destination
 * @param src Copy source
 * @param n Maximum number of characters bytes to copy
 * @return char* des
 */
char* strncpy(char* des, const char* src, size_t n);

/**
 * @brief Compare two strings
 * 
 * @param str1 String to compare
 * @param str2 String to compare
 * @return -1 if first different character in str1 is smaller than str2's, 1 if first different character in str1 is greater than str2's, 0 if both strings are the same
 */
int strcmp(const char* str1, const char* str2);

/**
 * @brief Compare two strings with limitation
 * 
 * @param str1 String to compare
 * @param str2 String to compare
 * @param n Maximum number of characters to compare
 * @return int -1 if first different character in str1 is smaller than str2's, 1 if first different character in str1 is greater than str2's, 0 if both strings are the same
 */
int strncmp(const char* str1, const char* str2, size_t n);

#endif // __KERNEL_STRING_H
