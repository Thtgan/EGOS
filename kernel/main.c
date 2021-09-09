#include<types.h>
#include<real/simpleAsmLines.h>

void kernel_main() {
    uint8_t* video = (uint8_t*)0xB8000;

    video[0] = 'H';
    video[2] = 'e';
    video[4] = 'l';
    video[6] = 'l';
    video[8] = 'o';

    die();

    video[10] = '!';
}