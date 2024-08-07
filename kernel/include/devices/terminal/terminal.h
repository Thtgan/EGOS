#if !defined(__TERMINAL_H)
#define __TERMINAL_H

#include<devices/terminal/inputBuffer.h>
#include<kit/types.h>
#include<structs/queue.h>
#include<multitask/semaphore.h>

#define TEXT_MODE_BUFFER_BEGIN              0xB8000
#define TEXT_MODE_WIDTH                     80 
#define TEXT_MODE_HEIGHT                    25
#define TEXT_MODE_SIZE                      TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT

typedef struct {
    Uint8 character, colorPattern;
} __attribute__((packed)) TerminalDisplayUnit;

//Bit 7 6 5 4 3 2 1 0
//    | | | | | | | |
//    | | | | | ^-^-^-- Foreground color
//    | | | | ^-------- Foreground color bright bit
//    | ^-^-^---------- Background color
//    ^---------------- Background color bright bit or enables blinking

#define TERMINAL_PATTERN_COLOR_BLACK           0
#define TERMINAL_PATTERN_COLOR_BLUE            1
#define TERMINAL_PATTERN_COLOR_GREEN           2
#define TERMINAL_PATTERN_COLOR_CYAN            3
#define TERMINAL_PATTERN_COLOR_RED             4
#define TERMINAL_PATTERN_COLOR_MAGENTA         5
#define TERMINAL_PATTERN_COLOR_BROWN           6
#define TERMINAL_PATTERN_COLOR_LIGHT_GRAY      7
#define TERMINAL_PATTERN_COLOR_DARK_GRAY       8
#define TERMINAL_PATTERN_COLOR_LIGHT_BLUE      9
#define TERMINAL_PATTERN_COLOR_LIGHT_GREEN     10
#define TERMINAL_PATTERN_COLOR_LIGHT_CYAN      11
#define TERMINAL_PATTERN_COLOR_LIGHT_RED       12
#define TERMINAL_PATTERN_COLOR_LIGHT_MAGNETA   13
#define TERMINAL_PATTERN_COLOR_YELLOW          14
#define TERMINAL_PATTERN_COLOR_WHITE           15

#define BUILD_PATTERN(__BACKGROUND_COLOR, __FOREGROUND_COLOR) (Uint8)VAL_OR(__FOREGROUND_COLOR, VAL_LEFT_SHIFT(__BACKGROUND_COLOR, 4))

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

typedef struct {
    Index16 loopRowBegin;                           //Which row in buffer does loop begin
    Size bufferRowSize;                           //Size of buffer (in row)
    char* buffer;                                   //Buffer

    Uint16 windowWidth, windowHeight, windowSize; //Window size
    Index16 windowRowBegin;                         //Which row in buffer does window

    Index16 rollRange;                              //Range of scrolling

    Semaphore outputLock;                           //Lock for output, used to handle output in multitask sence

    bool cursorLastInWindow, cursorEnabled;
    Index16 cursorPosX, cursorPosY;                 //Cursor position, starts from beginning of loop, (0, 0) means first character in first row of loop

    Uint8 colorPattern;
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
 * @return Result Result of the operation
 */
Result terminal_initStruct(Terminal* terminal, void* buffer, Size bufferSize, Size width, Size height);

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
 * @return Result Result of the operation
 */
Result terminal_scrollUp(Terminal* terminal);

/**
 * @brief Scroll down
 * 
 * @param terminal Terminal
 * @return Result Result of the operation
 */
Result terminal_scrollDown(Terminal* terminal);

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
 * @brief Set pattern of the output
 * 
 * @param terminal Terminal
 * @param background Output background color
 * @param foreground Output foreground color
 */
void terminal_setPattern(Terminal* terminal, Uint8 background, Uint8 foreground);

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

#endif // __TERMINAL_H