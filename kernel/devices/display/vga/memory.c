#include<devices/display/vga/memory.h>

#include<algorithms.h>
#include<devices/display/display.h>
#include<devices/display/vga/modes.h>
#include<devices/display/vga/vga.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging.h>

#define __VGA_MEMORY_POS_TO_INDEX(__MODE, __POS) ((__MODE)->header.width * (__POS)->x + (__POS)->y)

void vgaMemory_textWriteCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n) {
    Uint16 desIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);

    VGAtextModeCell* cellSrc = cells;
    VGAtextModeCell* cellDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAtextModeCell*)mode->header.videoMemory.begin) + desIndex);
    n = algorithms_umin16(n, mode->header.size - desIndex);
    for (; n > 0; --n, ++cellSrc, ++cellDes) {
        *cellDes = *cellSrc;
    }
}

void vgaMemory_textReadCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);

    VGAtextModeCell* cellSrc = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAtextModeCell*)mode->header.videoMemory.begin) + srcIndex);
    VGAtextModeCell* cellDes = cells;
    n = algorithms_umin16(n, mode->header.size - srcIndex);
    for (; n > 0; --n, ++cellSrc, ++cellDes) {
        *cellDes = *cellSrc;
    }
}

void vgaMemory_textSetCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell cell, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);

    VGAtextModeCell* cellDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAtextModeCell*)mode->header.videoMemory.begin) + srcIndex);
    n = algorithms_umin16(n, mode->header.size - srcIndex);
    for (; n > 0; --n, ++cellDes) {
        *cellDes = cell;
    }
}

void vgaMemory_textMoveCell(VGAtextMode* mode, DisplayPosition* des, DisplayPosition* src, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, src), desIndex = __VGA_MEMORY_POS_TO_INDEX(mode, des);
    VGAtextModeCell* cellSrc = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAtextModeCell*)mode->header.videoMemory.begin) + srcIndex);
    VGAtextModeCell* cellDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAtextModeCell*)mode->header.videoMemory.begin) + desIndex);

    memory_memmove(cellDes, cellSrc, (mode->header.size - algorithms_umax16(srcIndex, desIndex)) * sizeof(VGAtextModeCell));
}

void vgaMemory_linearWritePixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n) {
    Uint16 desIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);
    VGAcolor* pixelSrc = colors;
    VGAcolor* pixelDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAcolor*)mode->header.videoMemory.begin) + desIndex);
    n = algorithms_umin16(n, mode->header.size - desIndex);
    for (; n > 0; --n, ++pixelSrc, ++pixelDes) {
        *pixelDes = *pixelSrc;
    }
}

void vgaMemory_linearReadPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);
    VGAcolor* pixelSrc = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAcolor*)mode->header.videoMemory.begin) + srcIndex);
    VGAcolor* pixelDes = colors;
    n = algorithms_umin16(n, mode->header.size - srcIndex);
    for (; n > 0; --n, ++pixelSrc, ++pixelDes) {
        *pixelDes = *pixelSrc;
    }

}

void vgaMemory_linearSetPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor color, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, pos);
    VGAcolor* pixelDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAcolor*)mode->header.videoMemory.begin) + srcIndex);
    n = algorithms_umin16(n, mode->header.size - srcIndex);
    for (; n > 0; --n, ++pixelDes) {
        *pixelDes = color;
    }
}

void vgaMemory_linearMovePixel(VGAgraphicMode* mode, DisplayPosition* des, DisplayPosition* src, Size n) {
    Uint16 srcIndex = __VGA_MEMORY_POS_TO_INDEX(mode, src), desIndex = __VGA_MEMORY_POS_TO_INDEX(mode, des);
    VGAcolor* pixelSrc = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAcolor*)mode->header.videoMemory.begin) + srcIndex);
    VGAcolor* pixelDes = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(((VGAcolor*)mode->header.videoMemory.begin) + desIndex);

    memory_memmove(pixelDes, pixelSrc, (mode->header.size - algorithms_umax16(srcIndex, desIndex)) * sizeof(VGAcolor));
}