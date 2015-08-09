#ifndef BK_DNS_UTIL_H
#define BK_DNS_UTIL_H 1

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
  response += offset;

  do {
    n = *response++;
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name */
      break;
    } else {
      /* Not compressed name */
      while (n > 0) {
        if ((*query) != (*response)) {
          return 1;
        }
        ++response;
        ++query;
        --n;
      };
      ++query;
    }
  } while (*response != 0);

  return 0;
}


#endif
