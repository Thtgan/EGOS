#include<driver/vgaTextMode/textmode.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<real/simpleAsmLines.h>
#include<types.h>

__attribute__((section(".kernelMain")))
void kernelMain() {
    initTextMode();
    die();
}