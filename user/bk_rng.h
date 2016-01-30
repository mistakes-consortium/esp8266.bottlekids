#ifndef BK_RNG_H
#define BK_RNG_H 1

#include "bkmacros.h"
#include "debugging.h"

#include "c_types.h"


#define esp_hwrng_output_t uint32_t
#define bk_rng_bufsize_t uint8_t

// max is 64 for the var type we're using for pool size.
#define BK_RNG_BUF_CHUNKS 3
#define BK_RNG_BUF_BYTES BK_RNG_BUF_CHUNKS*sizeof(esp_hwrng_output_t)

inline esp_hwrng_output_t esp_hardware_rng() {
    return *(volatile esp_hwrng_output_t *)0x3FF20E44;
}

struct bk_rng_state {
    esp_hwrng_output_t pool[BK_RNG_BUF_CHUNKS];
    bk_rng_bufsize_t pool_offset;
};

void bk_rng_init(struct bk_rng_state *rng_state) {
    // empty pool - the offset is at the end of the buffer
    DEBUGUART("bk_rng_int: initializing empty pool\r\n");
    rng_state->pool_offset = BK_RNG_BUF_BYTES;
};

void bk_rng_fill(struct bk_rng_state *rng_state) {
    if (rng_state->pool_offset == 0) {
        DEBUGUART("bk_rng_fill: the pool is already full\r\n");
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

    DEBUGUART("bk_rng_fill: need to generate %u chunks.\r\n", need_to_generate);

    bk_rng_bufsize_t generate_offset;

    for (generate_offset = 0; generate_offset < need_to_generate; generate_offset++) {
        // generate need_to_generate chunks and store them
        rng_state->pool[generate_offset] = esp_hardware_rng();
    }

    // the pool is now usable from the start
    rng_state->pool_offset = 0;

    DEBUGUART("bk_rng_fill: finished generating, pool full.\r\n");
};

inline bk_rng_bufsize_t bk_rng_pool_size(struct bk_rng_state *rng_state) {
    return BK_RNG_BUF_BYTES - rng_state->pool_offset;
};

inline _bk_rng_buf_consume(struct bk_rng_state *rng_state, bk_rng_bufsize_t used_bytes) {
    rng_state->pool_offset += used_bytes;
    DEBUGUART("_bk_rng_buf_consume: consumed %u bytes from pool.\r\n", used_bytes);
};

uint8_t bk_rng_8bit(struct bk_rng_state *rng_state) {
    uint8_t ret;

    if (bk_rng_pool_size(rng_state) < sizeof(ret)) {
        DEBUGUART("bk_rng_8bit: filling pool.\r\n");
        bk_rng_fill(rng_state);
    }

    ret = ((uint8_t *)(rng_state->pool))[rng_state->pool_offset];
    _bk_rng_buf_consume(rng_state, 1);
    DEBUGUART("bk_rng_8bit: returning.\r\n");
    return ret;
};

#endif
