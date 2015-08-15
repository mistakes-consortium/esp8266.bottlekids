#ifndef BK_DNS_UTIL_H
#define BK_DNS_UTIL_H 1

inline unsigned char casefold(unsigned char c) {
    // subtract 0x20 from all octets in the inclusive range from 0x61 to 0x7A before comparing

    if ((c >= 0x61) && (c <= 0x7a)) {
        return c - 0x20;
    }
    return c;
}

inline
uint8_t
compare_chars(unsigned char a, unsigned char b) {
    if (casefold(a) == casefold(b)) {
        return 0;
    } else {
        return 1;
    }
}

// mostly taken from lwip core/dns.c, with modifications
size_t hostname_to_queryformat(char* name, char* query) {
    /*
    @param name char pointer to the start of a c string of the name to convert into query format and copy into the query buffer
    @param query pointer to location where the query formatted version of the name should be written to
    @returns number of bytes used in the query buffer by encoding the name
    */
    uint8_t n;
    char* nptr;
    size_t bytes_used = 0;
    name--;

    /* convert hostname into suitable query format. */
    do {
        ++name;
        nptr = query;
        ++query;
        bytes_used++; // nptr points to where the length will be stored. we have 'used' that.
        for(n = 0; *name != '.' && *name != 0; ++name) {
            *query = *name;
            bytes_used++; // we used a byte to store a byte of the name
            ++query;
            ++n;
        }
        // we write back to the start (it's length prefix encoded, unsigned 8 bit int)
        *nptr = n;
    } while(*name != 0);
    // write a 0 over the end, and then increment the query pointer
    // this means we're using another byte, I guess... but isn't the *nptr = n line already doing this?

    bytes_used++;
    *query++='\0';

    //DEBUGUART("name written to dns query using %d bytes.\r\n", bytes_used);

    return bytes_used;
}

// from lwip
/**
 * Compare the "dotted" name "query" with the encoded name "response"
 *
 * @param query hostname (not encoded) from the dns_table
 * @param response_ptr pointer to the start of the DNS response
 * @param response_size number of bytes in the dns response packet, used to ensure we don't read outside
                        the response message
 * @param offset offset of the encoded hostname in the DNS response. Assumed to be less than response_size
 * @return 0: names equal; 1: names differ
 */
static uint8_t
dns_compare_name(unsigned char *query, unsigned char *response_ptr, uint16_t response_size, uint16_t offset)
{
    unsigned char n;
    unsigned char* response = response_ptr;
    unsigned char* end = response_ptr + response_size;
    //response += offset;
    uint8_t compression_jumped = 1;

    //printf(">>>>>> begin!\n");

    do {
        // how many bytes and/or
        n = response[offset++];
        //printf("n = %d\n", n);
        if ((n & 0xc0) == 0xc0) {
            /** @see RFC 1035 - 4.1.4. Message compression */
            /* Compressed name */

            if ((offset >= response_size) || (offset == 0)) {
                //printf("<<<<<<<< pointer out of range (offset=%d). No match.\n", offset);
                return 1;
            }

            if (compression_jumped == 0) {
                //printf("<<<<<< you can only do one compression jump per name.\n");
                return 1;
            }
            compression_jumped = 0;

            uint16_t current_offset = offset - 1;
            offset = 255 * (0xc0 ^ n) + response[offset++];
            //printf("found offset to be %d after decoding pointer.\n", n);
            if (offset >= current_offset) {
                //printf("<<<<<<< forward pointers disallowed. No match. current offset=%d, jump=%d\n", current_offset, offset);
                return 1;
            }
        } else {
            /* Not compressed name */
            while (n > 0) {
                if (offset >= response_size) {
                    //printf("<<<<<< offset has grown larger than size of response. no match.\n");
                    return 1;
                }
                if (compare_chars(*query, response[offset]) != 0) {
                    //printf("<<<<<< a byte did not match. no match! Char in response was %c, in query was %c\n", response[offset], *query);
                    return 1;
                }
                //printf("a byte did match.. %c\n", *query);
                ++offset;
                ++query;
                --n;
            };
            ++query;

            if (offset >= response_size) {
                //printf("<<<<<< offset has grown larger than size of response. no match.\n");
                return 1;
            }
        }

    } while (response[offset] != 0);

    //printf("<<<<<< reached end. match!\n");
    return 0;
}


#endif
