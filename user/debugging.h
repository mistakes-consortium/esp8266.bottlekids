#ifndef BK_DEBUGGING_H
#define BK_DEBUGGING_H 1

char debugging_str[255];

#define DEBUGUART(...) os_sprintf((char*)(&(debugging_str[0])), __VA_ARGS__); uart0_sendStr(&(debugging_str[0]));

#ifndef min
// seriously???
#define min(a,b) ((a < b) ? a : b)
#endif

void print_hexdump(char *data, size_t bytes) {
    DEBUGUART("%d bytes:\r\n", bytes);

    uint16_t row;
    uint8_t spacing = 8;
    uint8_t per_row = 32;
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

                    printed++;
                } else {
                    DEBUGUART("%x%x", (c >> 4), (c % 0x10));
                }

                if ((i + 1) % spacing == 0) {
                    printed++;
                    uart0_sendStr(" ");
                }
            }

            if (mode == 0) {
                for (i = 0; i < (per_row + (per_row / spacing) - printed); i++) {
                    DEBUGUART("%s", " ");
                }
            }
        }

        DEBUGUART("%s", "\r\n");
    }
}

#endif
