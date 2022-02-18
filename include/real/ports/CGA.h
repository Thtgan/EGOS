#if !defined(__PORTS_CGA_H)
#define __PORTS_CGA_H

//CGA(Color Graphics Adapter)

#define CRT_INDEX   0x03D4
#define CRT_DATA    0x03D5

#define INDEX_HORIZONTAL_TOTAL              0x00
#define INDEX_HORIZONTAL_DISPLAYED          0x01
#define INDEX_HORIZONTAL_SYNC_POSITION      0x02
#define INDEX_HORIZONTAL_SYNC_PULSE_WIDTH   0x03
#define INDEX_VERTICAL_TOTAL                0x04
#define INDEX_VERTICAL_DISPLAYED            0x05
#define INDEX_VERTICAL_SYNC_POSITION        0x06
#define INDEX_VERTICAL_SYNC_PULSE_WIDTH     0x07
#define INDEX_INTERLACE_MODE                0x08
#define INDEX_MAXIMUM_SCAN_LINES            0x09
#define INDEX_CURSOR_START                  0x0A
#define INDEX_CURSOR_END                    0x0B
#define INDEX_START_ADDRESS_HIGH            0x0C
#define INDEX_START_ADDRESS_LOW             0x0D
#define INDEX_CURSOR_LOCATION_HIGH          0x0E
#define INDEX_CURSOR_LOCATION_LOW           0x0F
#define INDEX_LIGHT_PEN_HIGH                0x10
#define INDEX_LIGHT_PEN_LOW                 0x11

#endif // __PORTS_CGA_H
