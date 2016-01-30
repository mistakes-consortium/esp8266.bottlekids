#ifndef BK_DNS_H
#define BK_DNS_H 1

#include "bkmacros.h"

#include "c_types.h"
#include "user_interface.h"
#include "ip_addr.h"
#include "espconn.h"
#include "debugging.h"
#include "bk_rng.h"

bool bkdns_udp_setup = false;
struct espconn bkdns_udp_conn;
esp_udp bkdns_net_udp;


void dns_response_callback(void *arg, char *pdata, unsigned short len);

void BK_CACHEABLE bk_dns_init(unsigned char dnsnum) {
    if (bkdns_udp_setup) {
        return;
    }


    ip_addr_t ipa;
    ipa = dns_getserver(dnsnum);
    unsigned long ip = ipa.addr;

    DEBUGUART("dnsip: %d.%d.%d.%d\r\n", IP2STR(&ip));

    bkdns_udp_conn.type = ESPCONN_UDP;
    bkdns_udp_conn.state = ESPCONN_NONE;
    bkdns_udp_conn.proto.udp = &bkdns_net_udp;
    bkdns_udp_conn.proto.udp->local_port = espconn_port();
    bkdns_udp_conn.proto.udp->remote_port = 53;
    bkdns_udp_conn.recv_callback = dns_response_callback;
    DEBUGUART("DNS Source Port: %d Target Port: %d\r\n", bkdns_udp_conn.proto.udp->local_port, bkdns_udp_conn.proto.udp->remote_port);
    os_memcpy(bkdns_udp_conn.proto.udp->remote_ip, &ip, 4);

    if (espconn_create(&bkdns_udp_conn) == ESPCONN_OK) {
        bkdns_udp_setup = true;
    } else {
        uart0_sendStr("Failed to create DNS UDP connection...\r\n");
    }

}


// some code taken from lwip
/** DNS message header */
/*struct dns_hdr {
  PACK_STRUCT_FIELD(u16_t id);
  PACK_STRUCT_FIELD(u8_t flags1);
  PACK_STRUCT_FIELD(u8_t flags2);
  PACK_STRUCT_FIELD(u16_t numquestions);
  PACK_STRUCT_FIELD(u16_t numanswers);
  PACK_STRUCT_FIELD(u16_t numauthrr);
  PACK_STRUCT_FIELD(u16_t numextrarr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif
#define SIZEOF_DNS_HDR 12
*/

// taken from espressif/include/arch/cc.h in https://github.com/kadamski/esp-lwip.git
#define LWIP_PLATFORM_HTONS(_n)  ((uint16_t)((((_n) & 0xff) << 8) | (((_n) >> 8) & 0xff)))
#define LWIP_PLATFORM_HTONL(_n)  ((uint32_t)( (((_n) & 0xff) << 24) | (((_n) & 0xff00) << 8) | (((_n) >> 8)  & 0xff00) | (((_n) >> 24) & 0xff) ))

// headers
#define DHO_ID 0
#define DHO_FL1 2
#define DHO_FL2 3
#define DHO_NQST 4
#define DHO_NANS 6
#define DHO_NAUR 8
#define DHO_NEXR 10


// taken from lwip core/dns.c

#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

// taken from lwip src/include/lwip/dns.h, with modifications

/** DNS field TYPE used for "Resource Records" */
#define DNS_RRTYPE_A              1     /* a host address */
#define DNS_RRTYPE_NS             2     /* an authoritative name server */
#define DNS_RRTYPE_MD             3     /* a mail destination (Obsolete - use MX) */
#define DNS_RRTYPE_MF             4     /* a mail forwarder (Obsolete - use MX) */
#define DNS_RRTYPE_CNAME          5     /* the canonical name for an alias */
#define DNS_RRTYPE_SOA            6     /* marks the start of a zone of authority */
#define DNS_RRTYPE_MB             7     /* a mailbox domain name (EXPERIMENTAL) */
#define DNS_RRTYPE_MG             8     /* a mail group member (EXPERIMENTAL) */
#define DNS_RRTYPE_MR             9     /* a mail rename domain name (EXPERIMENTAL) */
#define DNS_RRTYPE_NULL           10    /* a null RR (EXPERIMENTAL) */
#define DNS_RRTYPE_WKS            11    /* a well known service description */
#define DNS_RRTYPE_PTR            12    /* a domain name pointer */
#define DNS_RRTYPE_HINFO          13    /* host information */
#define DNS_RRTYPE_MINFO          14    /* mailbox or mail list information */
#define DNS_RRTYPE_MX             15    /* mail exchange */
#define DNS_RRTYPE_TXT            16    /* text strings */
#define DNS_RRTYPE_SRV			  33    /* service records */

#define DNS_RRCLASS_IN            1     /* the Internet */
#define DNS_RRCLASS_CS            2     /* the CSNET class (Obsolete - used only for examples in some obsolete RFCs) */
#define DNS_RRCLASS_CH            3     /* the CHAOS class */
#define DNS_RRCLASS_HS            4     /* Hesiod [Dyer 87] */
#define DNS_RRCLASS_FLUSH         0x800 /* Flush bit */

#include "dns_util.h"

inline void write_uint16(char *buf, uint16_t v) {
    // alignment for 16 bit is 2 bytes, so we can't treat it that way unless we're lucky to be on
    // an aligned access
    uint16_t tmp16 = LWIP_PLATFORM_HTONS(v);
    os_memcpy(buf, (char *)(&tmp16), 2);
}

void BK_CACHEABLE bk_dns_query(uint8_t numdns, char *name) {
    /*
    TODO make this smart enough to allow multiple pending DNS queries, and actually parse
         the response fully, plus support more than just SRV...
    */
    if (!bkdns_udp_setup) {
        return;
    }

    // 18 bytes for header + question + other overhead, 256 bytes for name queried
    char sendbuf[274];
    char *sendbuf_ptr = (char *)(&sendbuf);

    os_memset(sendbuf, 0, 274);

    uint16_t seq = (uint16_t)esp_hardware_rng();

    // dns header - flags2 can be left alone and we are just doing 1 question
    write_uint16(sendbuf_ptr + DHO_ID, seq);
    *((uint8_t *)(&(sendbuf[DHO_FL1]))) = DNS_FLAG1_RD;
    write_uint16(sendbuf_ptr + DHO_NQST, 1);

    size_t bytes_used = 12;

    // name parameter
    bytes_used += bk_hostname_to_queryformat(name, &(sendbuf[bytes_used]));

    DEBUGUART("bk_dns_query: wrote hostname to query. bytes_used=%d\r\n", bytes_used);

    // query options
    sendbuf_ptr = (char *) &(sendbuf[bytes_used]);
    write_uint16(sendbuf_ptr, DNS_RRTYPE_SRV);
    write_uint16(sendbuf_ptr + 2, DNS_RRCLASS_IN);
    bytes_used += 4;

    DEBUGUART("bk_dns_query: dns packet of size %d\r\n", bytes_used);

    print_hexdump(sendbuf, bytes_used);

    char ret = espconn_sent(&bkdns_udp_conn, sendbuf, bytes_used);

    if (ret != ESPCONN_OK) {
        uart0_sendStr("bk_dns_query: Error Sending DNS query Packet...\r\n");
    } else {
        uart0_sendStr("bk_dns_query: Sent DNS query packet\r\n");
    }
}


void BK_CACHEABLE dns_response_callback(void *arg, char *pdata, unsigned short len) {
    DEBUGUART("%s", "UDP response:\r\n");
    print_hexdump(pdata, len);
}

#endif
