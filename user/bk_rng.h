#ifndef BK_RNG_H
#define BK_RNG_H 1

#include "c_types.h"
#include "osapi.h"

#include "bkmacros.h"
#include "debugging.h"


#define esp_hwrng_output_t uint32_t
#define bk_rng_bufsize_t uint16_t

// max is 16319 for the var type we're using for pool size due to integer math overflows
#define BK_RNG_BUF_CHUNKS 16
#define BK_RNG_BUF_BYTES BK_RNG_BUF_CHUNKS*sizeof(esp_hwrng_output_t)

inline esp_hwrng_output_t esp_hardware_rng() {
    // http://esp8266-re.foogod.com/wiki/Random_Number_Generator
    return *(volatile esp_hwrng_output_t *)0x3FF20E44;
}

struct bk_rng_state {
    esp_hwrng_output_t pool[BK_RNG_BUF_CHUNKS];
    bk_rng_bufsize_t pool_offset;
};

void bk_rng_init(struct bk_rng_state *rng_state) {
    // empty pool - the offset is at the end of the buffer
    //DEBUGUART("bk_rng_init: initializing empty pool\r\n");
    rng_state->pool_offset = BK_RNG_BUF_BYTES;
};

void bk_rng_fill(struct bk_rng_state *rng_state) {
    if (rng_state->pool_offset == 0) {
        //DEBUGUART("bk_rng_fill: the pool is already full\r\n");
        return;
    }

    bk_rng_bufsize_t need_to_generate;

    // always need to generate at least this much..
    need_to_generate = rng_state->pool_offset / sizeof(esp_hwrng_output_t);

    if (rng_state->pool_offset % sizeof(esp_hwrng_output_t) != 0) {
        // if the offset isn't aligned, we will also generate another round
        // to make up for the partially used chunk
        need_to_generate++;
    }

    //DEBUGUART("bk_rng_fill: need to generate %u chunks.\r\n", need_to_generate);

    bk_rng_bufsize_t generate_offset;

    for (generate_offset = 0; generate_offset < need_to_generate; generate_offset++) {
        // generate need_to_generate chunks and store them
        rng_state->pool[generate_offset] = esp_hardware_rng();
    }

    // the pool is now usable from the start
    rng_state->pool_offset = 0;

    //DEBUGUART("bk_rng_fill: finished generating, pool full.\r\n");
};

inline bk_rng_bufsize_t bk_rng_pool_size(struct bk_rng_state *rng_state) {
    return BK_RNG_BUF_BYTES - rng_state->pool_offset;
};

inline _bk_rng_buf_consume(struct bk_rng_state *rng_state, bk_rng_bufsize_t used_bytes) {
    rng_state->pool_offset += used_bytes;
    //DEBUGUART("_bk_rng_buf_consume: consumed %u bytes from pool.\r\n", used_bytes);
};

uint8_t bk_rng_8bit(struct bk_rng_state *rng_state) {
    uint8_t ret;

    if (bk_rng_pool_size(rng_state) < sizeof(ret)) {
        //DEBUGUART("bk_rng_8bit: filling pool.\r\n");
        bk_rng_fill(rng_state);
    }

    ret = ((uint8_t *)(rng_state->pool))[rng_state->pool_offset];
    _bk_rng_buf_consume(rng_state, sizeof(ret));
    //DEBUGUART("bk_rng_8bit: returning.\r\n");
    return ret;
};

uint16_t bk_rng_16bit(struct bk_rng_state *rng_state) {
    uint16_t ret;

    if (bk_rng_pool_size(rng_state) < sizeof(ret)) {
        //DEBUGUART("bk_rng_16bit: filling pool.\r\n");
        bk_rng_fill(rng_state);
    }

    os_memcpy((char *)(&ret), (char *)(rng_state->pool) + rng_state->pool_offset, sizeof(ret));
    _bk_rng_buf_consume(rng_state, sizeof(ret));

    //DEBUGUART("bk_rng_16bit: returning.\r\n");
    return ret;
};

uint32_t bk_rng_32bit(struct bk_rng_state *rng_state) {
    uint32_t ret;

    if (bk_rng_pool_size(rng_state) < sizeof(ret)) {
        //DEBUGUART("bk_rng_32bit: filling pool.\r\n");
        bk_rng_fill(rng_state);
    }

    os_memcpy((char *)(&ret), (char *)(rng_state->pool) + rng_state->pool_offset, sizeof(ret));
    _bk_rng_buf_consume(rng_state, sizeof(ret));

    //DEBUGUART("bk_rng_32bit: returning.\r\n");
    return ret;
};

uint64_t bk_rng_64bit(struct bk_rng_state *rng_state) {
    uint64_t ret;

    if (bk_rng_pool_size(rng_state) < sizeof(ret)) {
        //DEBUGUART("bk_rng_64bit: filling pool.\r\n");
        bk_rng_fill(rng_state);
    }

    os_memcpy((char *)(&ret), (char *)(rng_state->pool) + rng_state->pool_offset, sizeof(ret));
    _bk_rng_buf_consume(rng_state, sizeof(ret));

    //DEBUGUART("bk_rng_64bit: returning.\r\n");
    return ret;
};

void bk_rng_uuid4(struct bk_rng_state *rng_state, char *cstr_buffer) {
    /*
    cstr_buffer must be a valid memory pointer to a buffer with at least 37 bytes of space in it for writing the
    stringified uuid
    */

    uint32_t u1, u5;
    uint16_t u2, u3, u4, u6;

    u1 = bk_rng_32bit(rng_state);
    u5 = bk_rng_32bit(rng_state);

    u2 = bk_rng_16bit(rng_state);
    u3 = bk_rng_16bit(rng_state);
    u4 = bk_rng_16bit(rng_state);
    u6 = bk_rng_16bit(rng_state);

    u3 &= 0x0fff;
    u3 |= 0x4000;

    u4 &= 0b1011111111111111;
    u4 |= 0b1000000000000000;

    os_sprintf(cstr_buffer, "%08x-%04x-%04x-%04x-%08x%04x", u1, u2, u3, u4, u5, u6);
}

#endif
