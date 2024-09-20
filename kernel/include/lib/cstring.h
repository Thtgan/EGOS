#if !defined(__LIB_CSTRING_H)
#define __LIB_CSTRING_H

#include<kit/types.h>

/**
 * @brief Return the length of the string, ends with '\0'
 * 
 * @param str String
 * @return Size The length of the string
 */
Size cstring_strlen(ConstCstring str);

/**
 * @brief Return the first position in str1 where character not appeared in str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return Size First position in str1 where character not appeared in str2, length of str1 if all characters in str1 contained in str2
 */
Size cstring_strspn(ConstCstring str1, ConstCstring str2);

/**
 * @brief Return the first position in str1 where character appeared in str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return Size First position in str1 where character appeared in str2, length of str1 if all characters in str1 not contained in str2
 */
Size cstring_strcspn(ConstCstring str1, ConstCstring str2);

/**
 * @brief Return the pointer to the position str2 first appeared in str1
 * 
 * @param str1 String
 * @param str2 Pattern to search
 * @return Cstring Position str2 first appeared in str1, NULL if str2 not appeared in str1
 */
Cstring cstring_strstr(ConstCstring str1, ConstCstring str2);

/**
 * @brief Split string into tokens, '\0' will be added to end of the token
 * 
 * @param str String to truncate, if NULL, continue truncating previous string
 * @param delimiters Delimeters in str
 * @return Cstring The beginning of the token, NULL if token not exists
 */
Cstring cstring_strtok(Cstring str, ConstCstring delimiters);

/**
 * @brief Return the pointer to the first character both appeared in str1 and str2
 * 
 * @param str1 String
 * @param str2 Character set
 * @return Cstring Pointer to the first character both appeared in str1 and str2, NULL if all characters in str1 not appeared in str2
 */
Cstring cstring_strpbrk(ConstCstring str1, ConstCstring str2);

/**
 * @brief Return the first position where character appear in string
 * 
 * @param str String
 * @param ch Character to search
 * @return char* First position where character appear in string, NULL if character not appeared in string
 */
char* cstring_strchr(ConstCstring str, int ch);

/**
 * @brief Return the last position where character appear in string
 * 
 * @param str String
 * @param ch Character to search
 * @return char* Last position where character appear in string, NULL if character not appeared in string
 */
char* cstring_strrchr(ConstCstring str, int ch);

/**
 * @brief Copy string
 * 
 * @param des Copy destination
 * @param src Copy source
 * @return Cstring des
 */
Cstring cstring_strcpy(Cstring des, ConstCstring src);

/**
 * @brief Copy string with limitation
 * 
 * @param des Copy destination
 * @param src Copy source
 * @param n Maximum number of characters bytes to copy
 * @return Cstring des
 */
Cstring cstring_strncpy(Cstring des, ConstCstring src, Size n);

/**
 * @brief Compare two strings
 * 
 * @param str1 String to compare
 * @param str2 String to compare
 * @return -1 if first different character in str1 is smaller than str2's, 1 if first different character in str1 is greater than str2's, 0 if both strings are the same
 */
int cstring_strcmp(ConstCstring str1, ConstCstring str2);

/**
 * @brief Compare two strings with limitation
 * 
 * @param str1 String to compare
 * @param str2 String to compare
 * @param n Maximum number of characters to compare
 * @return int -1 if first different character in str1 is smaller than str2's, 1 if first different character in str1 is greater than str2's, 0 if both strings are the same
 */
int cstring_strncmp(ConstCstring str1, ConstCstring str2, Size n);

/**
 * @brief Get SDBM hash of the string
 * 
 * @param str String to hash
 * @return Size Hash value of string
 */
Size cstring_strhash(ConstCstring str);

#endif // __LIB_CSTRING_H
