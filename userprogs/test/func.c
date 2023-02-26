#include<func.h>

static int _arr[16];

void initFunc() {
    for (int i = 0; i < 16; ++i) {
        _arr[i] = i + 114514;
    }
}

int func(int index) {
    return _arr[index];
}