#include<devices/terminal/bootTTY.h>

#include<devices/display/display.h>
#include<devices/display/vga/vga.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<real/simpleAsmLines.h>
#include<algorithms.h>
#include<cstring.h>

static Size __bootTTY_read(Teletype* tty, void* buffer, Size n);

static Size __bootTTY_write(Teletype* tty, const void* buffer, Size n);

static void __bootTTY_flush(Teletype* tty);

static ConstCstring __bootTTY_findNewLine(const void* buffer, Size n);

static void __bootTTY_moveToNewLine(BootTeletype* tty);

static TeletypeOperations _virtualTTY_teletypeOperations = {
    .read = __bootTTY_read,
    .write = __bootTTY_write,
    .flush = __bootTTY_flush
};

void bootTeletype_initStruct(BootTeletype* tty) {
    DisplayContext* currentDisplayContext = display_getCurrentContext();
    if (currentDisplayContext->height != 25 || currentDisplayContext->width != 80) {    //debug_blowup is not available at this stage
        hlt();
        die();
    }

    tty->tty.operations = &_virtualTTY_teletypeOperations;
    tty->printedRowNum = 0;
    tty->lastRowLength = 0;

    void* videoBufferBegin = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V((void*)0xB8000);
    memory_memset(videoBufferBegin, 0, sizeof(VGAtextModeCell) * 25 * 80);
}

static Size __bootTTY_read(Teletype* tty, void* buffer, Size n) {
    return 0;
}

static Size __bootTTY_write(Teletype* tty, const void* buffer, Size n) {
    BootTeletype* bootTTY = HOST_POINTER(tty, BootTeletype, tty);

    DisplayPosition pos;
    ConstCstring currentBuffer = (ConstCstring)buffer;

    Size remainWriteByteNum = n;
    while (remainWriteByteNum > 0) {
        ConstCstring newLinePos = __bootTTY_findNewLine(buffer, n);
        Size lineByteNum = newLinePos == NULL ? remainWriteByteNum : (newLinePos - currentBuffer);
        Size remainLineByteNum = lineByteNum;

        while (remainLineByteNum > 0) {
            DEBUG_ASSERT_SILENT(bootTTY->lastRowLength <= 80);
            if (bootTTY->lastRowLength == 80) {
                __bootTTY_moveToNewLine(bootTTY);
            }

            Size writeByteNum = algorithms_umin64(remainLineByteNum, 80 - bootTTY->lastRowLength);

            DEBUG_ASSERT_SILENT(bootTTY->printedRowNum < 25);
            pos.x = bootTTY->printedRowNum;
            pos.y = bootTTY->lastRowLength;

            display_printString(&pos, currentBuffer, writeByteNum, 0xFFFFFF);
            bootTTY->lastRowLength += writeByteNum;

            remainLineByteNum -= writeByteNum;
            currentBuffer += writeByteNum;
        }

        remainWriteByteNum -= lineByteNum;
        if (newLinePos != NULL) {
            __bootTTY_moveToNewLine(bootTTY);
            --remainWriteByteNum;   //Count new line character in
            ++currentBuffer;
        }
    }
    
    return n;
}

static void __bootTTY_flush(Teletype* tty) {
    return;
}

static ConstCstring __bootTTY_findNewLine(const void* buffer, Size n) {
    ConstCstring str = (ConstCstring)buffer;
    for (int i = 0; i < n && str[i] != '\0'; ++i) {
        if (str[i] == '\n') {
            return str + i;
        }
    }
    return NULL;
}

static void __bootTTY_moveToNewLine(BootTeletype* tty) {
    if (tty->printedRowNum + 1 < 25) {
        ++tty->printedRowNum;
    } else {
        VGAtextModeCell* videoBufferBegin = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V((void*)0xB8000);
        memory_memmove(videoBufferBegin, videoBufferBegin + 80, sizeof(VGAtextModeCell) * 24 * 80);
        memory_memset(videoBufferBegin + 24 * 80, 0, sizeof(VGAtextModeCell) * 80);
    }
    tty->lastRowLength = 0;
}