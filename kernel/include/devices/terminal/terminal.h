#if !defined(__DEVICES_TERMINAL_TERMINAL_H)
#define __DEVICES_TERMINAL_TERMINAL_H

typedef struct Terminal Terminal;

#include<devices/display/display.h>
#include<devices/terminal/inputBuffer.h>
#include<kit/types.h>
#include<structs/queue.h>
#include<multitask/locks/semaphore.h>

/**
 * Window width
 * |-------------------------------------------------------------\
 * |                                                              \
 * +--------------------------------------------------------------+ <- buffer (Pointer)
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------+-----+ <- bufferRowBegin ------------------+
 * |                                                        |     |                                     |
 * |                                                        |     |                                     |
 * |                                          cursorPosX -> |     |                                     |
 * |                                                        |     |                                     |
 * |                                                        |     |                                     |
 * +--------------------------------------------------------+-----+-+ <- windowRowBegin ----+           |
 * |                                                        |     | |                       |           | Loop
 * |                                                        |     | |                       |           |
 * |                                                        |     | |                       |           |
 * |                                                        |     | |                       |           | <- rollRange
 * +--------------------------------------------------------+     | | <- windowHeight       | Window    |
 * |                             ^                          ^     | |                       |           |
 * |                             |                          |     | |                       |           |
 * |                       cursorPosY                    Cursor   | |                       |           |
 * |                                                              | |                       |           |
 * +--------------------------------------------------------------+-+-----------------------+           |
 * |                                                              |                                     |
 * |                                                              |                                     |
 * |                                                              |                                     |
 * +--------------------------------------------------------------+-------------------------------------+
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------------+
 * 
 * Buffer can be loop
 * Cursor ALWAYS stays in window
 * If cursor move backward, and it reaches window's begin, window moves backward, but stops when reaching buffer's begin
 * If cursor move forward, and it reaches window's end, window moves forward, if it reaches buffer's end, buffer moves forward too.
 */

typedef struct Terminal {
    Index16 loopRowBegin;                           //Which row in buffer does loop begin
    DisplayContext* displayContext;
    Size bufferSize, bufferRowSize;                 //Size of buffer (in row)

    char* buffer;                                   //Buffer
    Index16 windowRowBegin;                         //Which row in buffer does window

    Semaphore outputLock;                           //Lock for output, used to handle output in multitask sence

    bool cursorLastInWindow, cursorEnabled;
    Index16 cursorPosX, cursorPosY;                 //Cursor position, starts from beginning of loop, (0, 0) means first character in first row of loop

    Uint8 tabStride;

    bool inputMode;
    int inputLength;
    Semaphore inputLock;                            //Lock for input, used to block output when inputting
    InputBuffer inputBuffer;
} Terminal;

/**
 * @brief Initialize terminal
 * 
 * @param terminal Terminal struct
 * @param buffer Terminal buffer
 * @param bufferSize Size of the buffer
 * @param width Width of terminal on display
 * @param height Height of terminal on display
 */
void terminal_initStruct(Terminal* terminal, void* buffer, Size bufferSize);

void terminal_updateDisplayContext(Terminal* terminal);

/**
 * @brief Set current terminal
 * 
 * @param terminal Terminal to set
 */
void terminal_setCurrentTerminal(Terminal* terminal);

/**
 * @brief Get current terminal
 * 
 * @return Terminal* Current terminal
 */
Terminal* terminal_getCurrentTerminal();

/**
 * @brief Switch cursor enabled or disabled
 * 
 * @param terminal Terminal
 * @param enable Cursor enabled or disabled
 */
void terminal_switchCursor(Terminal* terminal, bool enable);

/**
 * @brief Refresh display to current terminal's buffer
 */
void terminal_flushDisplay();

/**
 * @brief Scroll up
 * 
 * @param terminal Terminal
 */
bool terminal_scrollUp(Terminal* terminal);

/**
 * @brief Scroll down
 * 
 * @param terminal Terminal
 */
bool terminal_scrollDown(Terminal* terminal);

/**
 * @brief Output string to terminal, support control characters
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminal_outputString(Terminal* terminal, ConstCstring str);

/**
 * @brief Output character to terminal, support control characters
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminal_outputChar(Terminal* terminal, char ch);

/**
 * @brief Set how mant spaces does a tab strides
 * 
 * @param terminal Terminal
 * @param stride Num of spaces
 */
void terminal_setTabStride(Terminal* terminal, Uint8 stride);

/**
 * @brief Perform effect of key home
 * 
 * @param terminal Terminal
 */
void terminal_cursorHome(Terminal* terminal);

/**
 * @brief Perform effect of key end
 * 
 * @param terminal Terminal
 */
void terminal_cursorEnd(Terminal* terminal);

/**
 * @brief Enable/disable input mode, initialized terminal has input mode disabled
 * 
 * @param terminal Terminal
 * @param enabled Enable input?
 */
void terminal_switchInput(Terminal* terminal, bool enabled);

/**
 * @brief Input string to terminal, nothing happens if input mode is disabled
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminal_inputString(Terminal* terminal, ConstCstring str);

/**
 * @brief Input character to terminal, nothing happens if input mode is disabled
 * 
 * @param terminal Terminal
 * @param ch Character
 */
void terminal_inputChar(Terminal* terminal, char ch);

/**
 * @brief Get a line input to terminal, ends with '\n'
 * 
 * @param terminal Terminal
 * @param buffer String buffer
 * @return int Num of character read
 */
int terminal_getline(Terminal* terminal, Cstring buffer);

/**
 * @brief Get a character input to terminal
 * 
 * @param terminal Terminal
 * @return int Character returned
 */
int terminal_getChar(Terminal* terminal);

// /**
//  * @brief Initialize terminal as a virtual device
//  * 
//  * @return FileOperations* Operation of virtual device file
//  */
// FileOperations* initTerminalDevice();

#endif // __DEVICES_TERMINAL_TERMINAL_H