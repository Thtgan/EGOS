#include<devices/display/display.h>

#include<algorithms.h>
#include<devices/display/vga/default.h>
#include<devices/display/vga/vga.h>
#include<kit/types.h>

//TODO: Add support for other video adapter

static DisplayContext _display_currentContext;
static bool _display_modeInitialized[DISPLAY_MODE_NUM] = { false };

OldResult display_init() {
    if (display_initMode(DISPLAY_MODE_DEFAULT) != RESULT_SUCCESS || display_switchMode(DISPLAY_MODE_DEFAULT) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

DisplayContext* display_getCurrentContext() {
    return &_display_currentContext;
}

OldResult display_initMode(DisplayMode mode) {
    if (mode >= DISPLAY_MODE_NUM) {
        return RESULT_ERROR;
    }

    if (_display_modeInitialized[mode]) {
        return RESULT_SUCCESS;
    }

    OldResult ret = RESULT_ERROR;
    switch (mode) {
        case DISPLAY_MODE_DEFAULT: {
            ret = RESULT_SUCCESS;
            break;
        }
        case DISPLAY_MODE_VGA: {
            ret = vga_init();
            break;
        }
        default: {
            ret = RESULT_ERROR;
            break;
        }
    }

    if (ret == RESULT_SUCCESS) {
        _display_modeInitialized[mode] = true;
    }

    return ret;
}

OldResult display_switchMode(DisplayMode mode) {
    if (mode >= DISPLAY_MODE_NUM || !_display_modeInitialized[mode]) {
        return RESULT_ERROR;
    }

    switch (mode) {
        case DISPLAY_MODE_DEFAULT: {
            vga_dumpDefaultDisplayContext(&_display_currentContext);
            break;
        }
        case DISPLAY_MODE_VGA: {
            vga_dumpDisplayContext(&_display_currentContext);
            break;
        }
        default: {
            return RESULT_ERROR;
        }
    }
    return RESULT_SUCCESS;
}

void display_drawPixel(DisplayPosition* position, RGBA color) {
    if (_display_currentContext.operations->drawPixel == NULL) {
        return;
    }
    _display_currentContext.operations->drawPixel(position, color);
}

RGBA display_readPixel(DisplayPosition* position) {
    if (_display_currentContext.operations->readPixel == NULL) {
        return 0x00000000;
    }
    return _display_currentContext.operations->readPixel(position);
}

void display_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    if (_display_currentContext.operations->drawLine == NULL) {
        return;
    }
    _display_currentContext.operations->drawLine(p1, p2, color);
}

void display_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    if (_display_currentContext.operations->fill == NULL) {
        return;
    }
    _display_currentContext.operations->fill(p1, p2, color);
}

void display_printCharacter(DisplayPosition* p1, Uint8 ch, RGBA color) {
    if (_display_currentContext.operations->printCharacter == NULL) {
        return;
    }
    _display_currentContext.operations->printCharacter(p1, ch, color);
}

void display_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color) {
    if (_display_currentContext.operations->printString == NULL) {
        return;
    }
    _display_currentContext.operations->printString(position, str, n, color);
}

void display_setCursorPosition(DisplayPosition* position) {
    if (_display_currentContext.operations->setCursorPosition == NULL) {
        return;
    }
    _display_currentContext.operations->setCursorPosition(position);
}

void display_switchCursor(bool enable) {
    if (_display_currentContext.operations->switchCursor == NULL) {
        return;
    }
    _display_currentContext.operations->switchCursor(enable);
}