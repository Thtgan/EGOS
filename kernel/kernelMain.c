#include<kit/includeBin.h>
#include<kit/incbin.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<real/simpleAsmLines.h>
#include<textmode.h>
#include<types.h>

__attribute__((section(".kernelMain")))
void kernelMain() {
    initTextMode();
    die();
}