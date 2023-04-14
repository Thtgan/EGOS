#if !defined(__TERMINAL_H)
#define __TERMINAL_H

#include<devices/terminal/inputBuffer.h>
#include<kit/types.h>
#include<structs/queue.h>
#include<multitask/semaphore.h>

typedef struct {
    uint8_t character, colorPattern;
} __attribute__((packed)) TerminalDisplayUnit;

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
    size_t bufferRowSize;                           //Size of buffer (in row)
    char* buffer;                                   //Buffer

    uint16_t windowWidth, windowHeight, windowSize; //Window size
    Index16 windowRowBegin;                         //Which row in buffer does window

    Index16 rollRange;                              //Range of scrolling

    Semaphore outputLock;                           //Lock for output, used to handle output in multitask sence

    bool cursorLastInWindow;
    Index16 cursorPosX, cursorPosY;                 //Cursor position, starts from beginning of loop, (0, 0) means first character in first row of loop

    uint8_t colorPattern;
    uint8_t tabStride;

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
 * @return bool Is terminal initialized successfully?
 */
bool initTerminal(Terminal* terminal, void* buffer, size_t bufferSize, size_t width, size_t height);

/**
 * @brief Set current terminal
 * 
 * @param terminal Terminal to set
 */
void setCurrentTerminal(Terminal* terminal);

/**
 * @brief Get current terminal
 * 
 * @return Terminal* Current terminal
 */
Terminal* getCurrentTerminal();

/**
 * @brief Refresh display to current terminal's buffer
 */
void displayFlush();

/**
 * @brief Scroll up
 * 
 * @param terminal Terminal
 * @return Is operation succeeded?
 */
bool terminalScrollUp(Terminal* terminal);

/**
 * @brief Scroll down
 * 
 * @param terminal Terminal
 * @return Is operation succeeded?
 */
bool terminalScrollDown(Terminal* terminal);

/**
 * @brief Output string to terminal, support control characters
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminalOutputString(Terminal* terminal, ConstCstring str);

/**
 * @brief Output character to terminal, support control characters
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminalOutputChar(Terminal* terminal, char ch);

/**
 * @brief Set pattern of the output
 * 
 * @param terminal Terminal
 * @param background Output background color
 * @param foreground Output foreground color
 */
void terminalSetPattern(Terminal* terminal, uint8_t background, uint8_t foreground);

/**
 * @brief Set how mant spaces does a tab strides
 * 
 * @param terminal Terminal
 * @param stride Num of spaces
 */
void terminalSetTabStride(Terminal* terminal, uint8_t stride);

/**
 * @brief Perform effect of key home
 * 
 * @param terminal Terminal
 */
void terminalCursorHome(Terminal* terminal);

/**
 * @brief Perform effect of key end
 * 
 * @param terminal Terminal
 */
void terminalCursorEnd(Terminal* terminal);

/**
 * @brief Enable/disable input mode, initialized terminal has input mode disabled
 * 
 * @param terminal Terminal
 * @param enabled Enable input?
 */
void terminalSwitchInput(Terminal* terminal, bool enabled);

/**
 * @brief Input string to terminal, nothing happens if input mode is disabled
 * 
 * @param terminal Terminal
 * @param str String
 */
void terminalInputString(Terminal* terminal, ConstCstring str);

/**
 * @brief Input character to terminal, nothing happens if input mode is disabled
 * 
 * @param terminal Terminal
 * @param ch Character
 */
void terminalInputChar(Terminal* terminal, char ch);

/**
 * @brief Get a line input to terminal, ends with '\n'
 * 
 * @param terminal Terminal
 * @param buffer String buffer
 * @return int Num of character read
 */
int terminalGetline(Terminal* terminal, Cstring buffer);

/**
 * @brief Get a character input to terminal
 * 
 * @param terminal Terminal
 * @return int Character returned
 */
int terminalGetChar(Terminal* terminal);

#endif // __TERMINAL_H