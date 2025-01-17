#include<devices/display/display.h>

#include<algorithms.h>
#include<devices/display/vga/default.h>
#include<devices/display/vga/vga.h>
#include<kit/types.h>
#include<result.h>

//TODO: Add support for other video adapter

static DisplayContext _display_currentContext;
static bool _display_modeInitialized[DISPLAY_MODE_NUM] = { false };

Result* display_init() {
    ERROR_TRY_CATCH_DIRECT(
        display_initMode(DISPLAY_MODE_DEFAULT),
        ERROR_CATCH_DEFAULT_CODES_PASS
    );
    
    ERROR_TRY_CATCH_DIRECT(
        display_switchMode(DISPLAY_MODE_DEFAULT),
        ERROR_CATCH_DEFAULT_CODES_PASS
    );

    ERROR_RETURN_OK();
}

DisplayContext* display_getCurrentContext() {
    return &_display_currentContext;
}

Result* display_initMode(DisplayMode mode) {
    if (mode >= DISPLAY_MODE_NUM) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS);
    }

    if (_display_modeInitialized[mode]) {
        ERROR_RETURN_OK();
    }

    switch (mode) {
        case DISPLAY_MODE_DEFAULT: {
            break;
        }
        case DISPLAY_MODE_VGA: {
            ERROR_TRY_CATCH_DIRECT(
                vga_init(),
                ERROR_CATCH_DEFAULT_CODES_PASS
            );
            break;
        }
        default: {
            ERROR_THROW(ERROR_ID_UNKNOWN);
        }
    }

    _display_modeInitialized[mode] = true;
    ERROR_RETURN_OK();
}

Result* display_switchMode(DisplayMode mode) {
    if (!_display_modeInitialized[mode]) {
        ERROR_THROW(ERROR_ID_STATE_ERROR);
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
            ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS);
        }
    }
    ERROR_RETURN_OK();
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