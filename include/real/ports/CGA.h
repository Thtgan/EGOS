#if !defined(__PORTS_CGA_H)
#define __PORTS_CGA_H

//CGA(Color Graphics Adapter)

//CGA CRT register select index port
#define CGA_CRT_INDEX                   0x03D4

//CRT indices
#define CGA_HORIZONTAL_TOTAL            0x00
#define CGA_HORIZONTAL_DISPLAYED        0x01
#define CGA_HORIZONTAL_SYNC_POSITION    0x02
#define CGA_HORIZONTAL_SYNC_PULSE_WIDTH 0x03
#define CGA_VERTICAL_TOTAL              0x04
#define CGA_VERTICAL_DISPLAYED          0x05
#define CGA_VERTICAL_SYNC_POSITION      0x06
#define CGA_VERTICAL_SYNC_PULSE_WIDTH   0x07
#define CGA_INTERLACE_MODE              0x08
#define CGA_MAXIMUM_SCAN_LINES          0x09
#define CGA_CURSOR_START                0x0A
#define CGA_CURSOR_END                  0x0B
#define CGA_START_ADDRESS_HIGH          0x0C
#define CGA_START_ADDRESS_LOW           0x0D
#define CGA_CURSOR_LOCATION_HIGH        0x0E
#define CGA_CURSOR_LOCATION_LOW         0x0F
#define CGA_LIGHT_PEN_HIGH              0x10
#define CGA_LIGHT_PEN_LOW               0x11

//CGA CRT data port
#define CGA_CRT_DATA                    0x03D5

#endif // __PORTS_CGA_H
