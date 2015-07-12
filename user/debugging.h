#ifndef BK_DEBUGGING_H
#define BK_DEBUGGING_H 1

char debugging_str[255];

#define DEBUGUART(pattern, ...) os_sprintf((char*)(&(debugging_str[0])), pattern, __VA_ARGS__); uart0_sendStr(&(debugging_str[0]));

#ifndef min
// seriously???
#define min(a,b) ((a < b) ? a : b)
#endif

void print_hexdump(char* data, size_t bytes) {
    DEBUGUART("%d bytes:\r\n", bytes);

    uint16_t row;
    uint8_t per_row = 16;
    uint16_t i;
    int mode = 0;
    int printed;
    for (row = 0; row <= (bytes / per_row); row++) {
        uint16_t data_this_row = min((bytes - (row * per_row)), per_row);
        //DEBUGUART("data entries this row: %d\r\n", data_this_row);

        for (mode = 0; mode < 2; mode++) {
            printed = 0;
            for (i = 0; i < data_this_row; i++) {
                unsigned char c = data[row * per_row + i];
                if (mode == 0) {
                    if ((c >= 0x20) && (c <= 0x7e)) {
                        DEBUGUART("%c", c);
                    } else {
                        DEBUGUART(".", c)
                    }
                } else {
                    printed++;
                    DEBUGUART("%x%x", (c >> 4), (c % 0x10));
                }
            }
            //DEBUGUART("\r\n I printed %d bytes of hex.\r\n", printed);

            for (; i < per_row; i++) {
                if (mode == 0) {
                    DEBUGUART("%s", " ");
                } else {
                    DEBUGUART("%s", "   ");
                }
            }

            if (mode == 0) {
                DEBUGUART("%s", "  ");
            }
        }
        DEBUGUART("%s", "\r\n");
    }
}

#endif
