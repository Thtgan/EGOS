#include<blowup.h>

#include<stdio.h>
#include<stdlib.h>

void blowup(const char* info) {
    printf("%s\n", info);
    abort();
}