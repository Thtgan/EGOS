#include<func.h>

int stoi(const char* str) {
    int ret = 0;
    for (; *str != '\0'; ++str) {
        int num = *str - '0';
        if (!(0 <= num && num <= 9)) {
            return 0;
        }
        ret = ret * 10 + num;
    }

    return ret;
}

int strncmp(const char* str1, const char* str2, unsigned long long n) {
    for (int i = 1; i < n && *str1 == *str2 && *str1 != '\0'; ++i) {
        ++str1, ++str2;
    }
    
    return *str1 - *str2;
}