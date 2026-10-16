#include "checksums.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* =========================================================================
 * Generic CRC & Table-driven Helpers
 * ========================================================================= */

static uint8_t reflect8(uint8_t val) {
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        if (val & (1 << i)) res |= (1 << (7 - i));
    }
    return res;
}

/* =========================================================================
 * MD5 Implementation (RFC 1321)
 * ========================================================================= */

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buffer[64];
} md5_ctx_t;

#define F_MD5(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G_MD5(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H_MD5(x, y, z) ((x) ^ (y) ^ (z))
#define I_MD5(x, y, z) ((y) ^ ((x) | (~z)))
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
 (a) += F_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I_MD5 ((b), (c), (d)) + (x) + (uint32_t)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    for (int i = 0, j = 0; j < 64; i++, j += 4)
        x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j+1]) << 8) |
               (((uint32_t)block[j+2]) << 16) | (((uint32_t)block[j+3]) << 24);

    /* Round 1 */
    FF (a, b, c, d, x[ 0], 7, 0xd76aa478);
    FF (d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF (c, d, a, b, x[ 2], 17, 0x242070db);
    FF (b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF (a, b, c, d, x[ 4], 7, 0xf57c0faf);
    FF (d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF (c, d, a, b, x[ 6], 17, 0xa8304613);
    FF (b, c, d, a, x[ 7], 22, 0xfd469501);
    FF (a, b, c, d, x[ 8], 7, 0x698098d8);
    FF (d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF (c, d, a, b, x[10], 17, 0xffff5bb1);
    FF (b, c, d, a, x[11], 22, 0x895cd7be);
    FF (a, b, c, d, x[12], 7, 0x6b901122);
    FF (d, a, b, c, x[13], 12, 0xfd987193);
    FF (c, d, a, b, x[14], 17, 0xa679438e);
    FF (b, c, d, a, x[15], 22, 0x49b40821);

    /* Round 2 */
    GG (a, b, c, d, x[ 1], 5, 0xf61e2562);
    GG (d, a, b, c, x[ 6], 9, 0xc040b340);
    GG (c, d, a, b, x[11], 14, 0x265e5a51);
    GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG (a, b, c, d, x[ 5], 5, 0xd62f105d);
    GG (d, a, b, c, x[10], 9,  0x2441453);
    GG (c, d, a, b, x[15], 14, 0xd8a1e681);
    GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG (a, b, c, d, x[ 9], 5, 0x21e1cde6);
    GG (d, a, b, c, x[14], 9, 0xc33707d6);
    GG (c, d, a, b, x[ 3], 14, 0xf4d50d87);
    GG (b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG (a, b, c, d, x[13], 5, 0xa9e3e905);
    GG (d, a, b, c, x[ 2], 9, 0xfcefa3f8);
    GG (c, d, a, b, x[ 7], 14, 0x676f02d9);
    GG (b, c, d, a, x[12], 20, 0x8d2a4c8a);

    /* Round 3 */
    HH (a, b, c, d, x[ 5], 4, 0xfffa3942);
    HH (d, a, b, c, x[ 8], 11, 0x8771f681);
    HH (c, d, a, b, x[11], 16, 0x6d9d6122);
    HH (b, c, d, a, x[14], 23, 0xfde5380c);
    HH (a, b, c, d, x[ 1], 4, 0xa4beea44);
    HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60);
    HH (b, c, d, a, x[10], 23, 0xbebfbc70);
    HH (a, b, c, d, x[13], 4, 0x289b7ec6);
    HH (d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH (c, d, a, b, x[ 3], 16, 0xd4ef3085);
    HH (b, c, d, a, x[ 6], 23,  0x4881d05);
    HH (a, b, c, d, x[ 9], 4, 0xd9d4d039);
    HH (d, a, b, c, x[12], 11, 0xe6db99e5);
    HH (c, d, a, b, x[15], 16, 0x1fa27cf8);
    HH (b, c, d, a, x[ 2], 23, 0xc4ac5665);

    /* Round 4 */
    II (a, b, c, d, x[ 0], 6, 0xf4292244);
    II (d, a, b, c, x[ 7], 10, 0x432aff97);
    II (c, d, a, b, x[14], 15, 0xab9423a7);
    II (b, c, d, a, x[ 5], 21, 0xfc93a039);
    II (a, b, c, d, x[12], 6, 0x655b59c3);
    II (d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II (c, d, a, b, x[10], 15, 0xffeff47d);
    II (b, c, d, a, x[ 1], 21, 0x85845dd1);
    II (a, b, c, d, x[ 8], 6, 0x6fa87e4f);
    II (d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II (c, d, a, b, x[ 6], 15, 0xa3014314);
    II (b, c, d, a, x[13], 21, 0x4e0811a1);
    II (a, b, c, d, x[ 4], 6, 0xf7537e82);
    II (d, a, b, c, x[11], 10, 0xbd3af235);
    II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb);
    II (b, c, d, a, x[ 9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void md5_init(md5_ctx_t *ctx) {
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void md5_update(md5_ctx_t *ctx, const uint8_t *input, size_t inputLen) {
    size_t i, index, partLen;
    index = (size_t)((ctx->count[0] >> 3) & 0x3F);
    if ((ctx->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3))
        ctx->count[1]++;
    ctx->count[1] += ((uint32_t)inputLen >> 29);
    partLen = 64 - index;
    if (inputLen >= partLen) {
        memcpy(&ctx->buffer[index], input, partLen);
        md5_transform(ctx->state, ctx->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64)
            md5_transform(ctx->state, &input[i]);
        index = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[index], &input[i], inputLen - i);
}

static void md5_final(uint8_t digest[16], md5_ctx_t *ctx) {
    static const uint8_t PADDING[64] = { 0x80 };
    uint8_t bits[8];
    for (int i = 0, j = 0; j < 8; i++, j += 4) {
        bits[j] = (uint8_t)(ctx->count[i] & 0xFF);
        bits[j+1] = (uint8_t)((ctx->count[i] >> 8) & 0xFF);
        bits[j+2] = (uint8_t)((ctx->count[i] >> 16) & 0xFF);
        bits[j+3] = (uint8_t)((ctx->count[i] >> 24) & 0xFF);
    }
    size_t index = (size_t)((ctx->count[0] >> 3) & 0x3f);
    size_t padLen = (index < 56) ? (56 - index) : (120 - index);
    md5_update(ctx, PADDING, padLen);
    md5_update(ctx, bits, 8);
    for (int i = 0, j = 0; j < 16; i++, j += 4) {
        digest[j] = (uint8_t)(ctx->state[i] & 0xFF);
        digest[j+1] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        digest[j+2] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        digest[j+3] = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
    }
}

/* =========================================================================
 * SHA-1 Implementation (FIPS 180-1)
 * ========================================================================= */

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t  buffer[64];
} sha1_ctx_t;

#define SHA1_ROL(val, bits) (((val) << (bits)) | ((val) >> (32 - (bits))))

static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t w[80];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)buffer[i * 4] << 24) | ((uint32_t)buffer[i * 4 + 1] << 16) |
               ((uint32_t)buffer[i * 4 + 2] << 8) | (uint32_t)buffer[i * 4 + 3];
    }
    for (int i = 16; i < 80; i++) {
        w[i] = SHA1_ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = SHA1_ROL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1_init(sha1_ctx_t *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
}

static void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, size_t len) {
    size_t i, j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += (uint32_t)len << 3) < ((uint32_t)len << 3)) ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);
    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) sha1_transform(ctx->state, &data[i]);
        j = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(uint8_t digest[20], sha1_ctx_t *ctx) {
    uint8_t finalcount[8];
    for (int i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
    }
    uint8_t c = 0x80;
    sha1_update(ctx, &c, 1);
    while ((ctx->count[0] & 504) != 448) {
        c = 0x00;
        sha1_update(ctx, &c, 1);
    }
    sha1_update(ctx, finalcount, 8);
    for (int i = 0; i < 20; i++) {
        digest[i] = (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
}

/* =========================================================================
 * SHA-256 Implementation (FIPS 180-2)
 * ========================================================================= */

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[64];
} sha256_ctx_t;

#define SHA256_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHA256_CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define SHA256_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA256_EP0(x) (SHA256_ROTR(x, 2) ^ SHA256_ROTR(x, 13) ^ SHA256_ROTR(x, 22))
#define SHA256_EP1(x) (SHA256_ROTR(x, 6) ^ SHA256_ROTR(x, 11) ^ SHA256_ROTR(x, 25))
#define SHA256_SIG0(x) (SHA256_ROTR(x, 7) ^ SHA256_ROTR(x, 18) ^ ((x) >> 3))
#define SHA256_SIG1(x) (SHA256_ROTR(x, 17) ^ SHA256_ROTR(x, 19) ^ ((x) >> 10))

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    uint32_t m[64];
    for (int i = 0; i < 16; i++) {
        m[i] = ((uint32_t)data[i * 4] << 24) | ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) | (uint32_t)data[i * 4 + 3];
    }
    for (int i = 16; i < 64; i++) {
        m[i] = SHA256_SIG1(m[i - 2]) + m[i - 7] + SHA256_SIG0(m[i - 15]) + m[i - 16];
    }
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + SHA256_EP1(e) + SHA256_CH(e, f, g) + sha256_k[i] + m[i];
        uint32_t t2 = SHA256_EP0(a) + SHA256_MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->buffer[ctx->count % 64] = data[i];
        ctx->count++;
        if (ctx->count % 64 == 0) {
            sha256_transform(ctx->state, ctx->buffer);
        }
    }
}

static void sha256_final(uint8_t digest[32], sha256_ctx_t *ctx) {
    uint64_t total_bits = ctx->count * 8;
    size_t rem = ctx->count % 64;
    ctx->buffer[rem++] = 0x80;
    if (rem > 56) {
        while (rem < 64) ctx->buffer[rem++] = 0;
        sha256_transform(ctx->state, ctx->buffer);
        rem = 0;
    }
    while (rem < 56) ctx->buffer[rem++] = 0;
    for (int i = 7; i >= 0; i--) {
        ctx->buffer[rem++] = (uint8_t)((total_bits >> (i * 8)) & 0xFF);
    }
    sha256_transform(ctx->state, ctx->buffer);
    for (int i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFF);
    }
}

/* =========================================================================
 * Full Checksum Calculation Engine
 * ========================================================================= */

typedef struct {
    // Simple
    uint64_t sum;
    uint8_t  xor8;
    uint32_t adler_a;
    uint32_t adler_b;
    uint16_t fletcher16_sum1;
    uint16_t fletcher16_sum2;
    uint32_t fletcher32_sum1;
    uint32_t fletcher32_sum2;

    // CRC-8 models
    uint8_t crc8_std;
    uint8_t crc8_cdma;
    uint8_t crc8_darc;
    uint8_t crc8_dvb;
    uint8_t crc8_ebu;
    uint8_t crc8_icode;
    uint8_t crc8_itu;
    uint8_t crc8_maxim;
    uint8_t crc8_rohc;
    uint8_t crc8_wcdma;

    // CRC-16 models
    uint16_t crc16_arc;
    uint16_t crc16_ccitt;
    uint16_t crc16_modbus;
    uint16_t crc16_xmodem;

    // CRC-32
    uint32_t crc32_ieee;
    uint32_t crc32_bzip2;

    // CRC-64
    uint64_t crc64_ecma;
    uint64_t crc64_iso;

    // FNV
    uint32_t fnv1_32;
    uint32_t fnv1a_32;
    uint64_t fnv1_64;
    uint64_t fnv1a_64;

    // Hashes
    md5_ctx_t    md5;
    sha1_ctx_t   sha1;
    sha256_ctx_t sha256;
} calc_accum_t;

/* Update generic 8-bit CRC */
static uint8_t crc8_update_custom(uint8_t current, uint8_t byte, uint8_t poly, bool refin) {
    uint8_t d = refin ? reflect8(byte) : byte;
    current ^= d;
    for (int i = 0; i < 8; i++) {
        if (current & 0x80) current = (current << 1) ^ poly;
        else current <<= 1;
    }
    return current;
}

/* Update generic 16-bit CRC */
static uint16_t crc16_update_custom(uint16_t current, uint8_t byte, uint16_t poly, bool refin) {
    if (refin) {
        current ^= byte;
        for (int i = 0; i < 8; i++) {
            if (current & 0x0001) current = (current >> 1) ^ poly;
            else current >>= 1;
        }
    } else {
        current ^= ((uint16_t)byte << 8);
        for (int i = 0; i < 8; i++) {
            if (current & 0x8000) current = (current << 1) ^ poly;
            else current <<= 1;
        }
    }
    return current;
}

static uint32_t crc32_update_ieee(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return crc;
}

static uint64_t crc64_update_ecma(uint64_t crc, uint8_t byte) {
    crc ^= ((uint64_t)byte << 56);
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000000000000000ULL) {
            crc = (crc << 1) ^ 0x42F0E1EBA9EA3693ULL;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

checksum_list_t *checksums_calculate(const piece_table_t *pt, hxd_range_t range, hxd_endianness_t endian) {
    (void)endian;
    if (!pt) return NULL;

    hxd_offset_t start = range.length > 0 ? range.start : 0;
    hxd_offset_t total_len = range.length > 0 ? range.length : pt_get_size(pt);
    if (start >= pt_get_size(pt)) return NULL;
    if (start + total_len > pt_get_size(pt)) total_len = pt_get_size(pt) - start;

    calc_accum_t acc;
    memset(&acc, 0, sizeof(acc));

    acc.adler_a = 1;
    acc.crc8_cdma = 0xFF;
    acc.crc8_ebu  = 0xFF;
    acc.crc8_icode = 0xFD;
    acc.crc8_rohc = 0xFF;
    acc.crc16_ccitt = 0xFFFF;
    acc.crc16_modbus = 0xFFFF;
    acc.crc32_ieee = 0xFFFFFFFF;
    acc.crc32_bzip2 = 0xFFFFFFFF;
    acc.crc64_ecma = 0;
    acc.crc64_iso = 0;
    acc.fnv1_32  = 0x811c9dc5;
    acc.fnv1a_32 = 0x811c9dc5;
    acc.fnv1_64  = 0xcbf29ce484222325ULL;
    acc.fnv1a_64 = 0xcbf29ce484222325ULL;

    md5_init(&acc.md5);
    sha1_init(&acc.sha1);
    sha256_init(&acc.sha256);

    uint8_t buffer[65536];
    hxd_offset_t processed = 0;

    while (processed < total_len) {
        hxd_size_t chunk = sizeof(buffer);
        if (processed + chunk > total_len) {
            chunk = total_len - processed;
        }

        pt_read_bytes(pt, start + processed, buffer, chunk, NULL);

        for (hxd_size_t i = 0; i < chunk; i++) {
            uint8_t b = buffer[i];

            // Simple
            acc.sum += b;
            acc.xor8 ^= b;
            acc.adler_a = (acc.adler_a + b) % 65521;
            acc.adler_b = (acc.adler_b + acc.adler_a) % 65521;

            acc.fletcher16_sum1 = (acc.fletcher16_sum1 + b) % 255;
            acc.fletcher16_sum2 = (acc.fletcher16_sum2 + acc.fletcher16_sum1) % 255;
            acc.fletcher32_sum1 = (acc.fletcher32_sum1 + b) % 65535;
            acc.fletcher32_sum2 = (acc.fletcher32_sum2 + acc.fletcher32_sum1) % 65535;

            // CRC-8 variants
            acc.crc8_std   = crc8_update_custom(acc.crc8_std, b, 0x07, false);
            acc.crc8_cdma  = crc8_update_custom(acc.crc8_cdma, b, 0x9B, false);
            acc.crc8_darc  = crc8_update_custom(acc.crc8_darc, b, 0x39, true);
            acc.crc8_dvb   = crc8_update_custom(acc.crc8_dvb, b, 0xD5, false);
            acc.crc8_ebu   = crc8_update_custom(acc.crc8_ebu, b, 0x1D, true);
            acc.crc8_icode = crc8_update_custom(acc.crc8_icode, b, 0x1D, false);
            acc.crc8_itu   = crc8_update_custom(acc.crc8_itu, b, 0x07, false);
            acc.crc8_maxim = crc8_update_custom(acc.crc8_maxim, b, 0x31, true);
            acc.crc8_rohc  = crc8_update_custom(acc.crc8_rohc, b, 0x07, true);
            acc.crc8_wcdma = crc8_update_custom(acc.crc8_wcdma, b, 0x9B, true);

            // CRC-16 variants
            acc.crc16_arc    = crc16_update_custom(acc.crc16_arc, b, 0xA001, true);
            acc.crc16_ccitt  = crc16_update_custom(acc.crc16_ccitt, b, 0x1021, false);
            acc.crc16_modbus = crc16_update_custom(acc.crc16_modbus, b, 0xA001, true);
            acc.crc16_xmodem = crc16_update_custom(acc.crc16_xmodem, b, 0x1021, false);

            // CRC-32
            acc.crc32_ieee  = crc32_update_ieee(acc.crc32_ieee, b);
            acc.crc32_bzip2 = crc8_update_custom((uint8_t)(acc.crc32_bzip2 >> 24), b, 0x04, false); // placeholder

            // CRC-64
            acc.crc64_ecma = crc64_update_ecma(acc.crc64_ecma, b);

            // FNV
            acc.fnv1_32 = (acc.fnv1_32 * 0x01000193) ^ b;
            acc.fnv1a_32 = (acc.fnv1a_32 ^ b) * 0x01000193;
            acc.fnv1_64 = (acc.fnv1_64 * 0x00000100000001B3ULL) ^ b;
            acc.fnv1a_64 = (acc.fnv1a_64 ^ b) * 0x00000100000001B3ULL;
        }

        md5_update(&acc.md5, buffer, chunk);
        sha1_update(&acc.sha1, buffer, chunk);
        sha256_update(&acc.sha256, buffer, chunk);

        processed += chunk;
    }

    // Finalize
    uint32_t adler32 = (acc.adler_b << 16) | acc.adler_a;
    uint16_t fletcher16 = (acc.fletcher16_sum2 << 8) | acc.fletcher16_sum1;
    uint32_t fletcher32 = (acc.fletcher32_sum2 << 16) | acc.fletcher32_sum1;
    uint32_t crc32_final = acc.crc32_ieee ^ 0xFFFFFFFF;
    acc.crc8_itu ^= 0x55;

    uint8_t md5_res[16];
    md5_final(md5_res, &acc.md5);

    uint8_t sha1_res[20];
    sha1_final(sha1_res, &acc.sha1);

    uint8_t sha256_res[32];
    sha256_final(sha256_res, &acc.sha256);

    // Populate results list matching myhex.online categories
    checksum_list_t *list = calloc(1, sizeof(checksum_list_t));
    list->items = calloc(25, sizeof(checksum_result_t));

    #define ADD_ITEM(i_id, i_grp, i_name, i_desc, val_hex, val_dec) do { \
        checksum_result_t *it = &list->items[list->count++]; \
        it->id = i_id; it->group = i_grp; it->name = i_name; it->description = i_desc; \
        snprintf(it->result_hex, sizeof(it->result_hex), "%s", val_hex); \
        snprintf(it->result_dec, sizeof(it->result_dec), "%s", val_dec); \
    } while(0)

    char hexbuf[128], decbuf[128];

    // Simple
    snprintf(hexbuf, sizeof(hexbuf), "%02X", (uint8_t)(acc.sum & 0xFF));
    snprintf(decbuf, sizeof(decbuf), "%u", (uint8_t)(acc.sum & 0xFF));
    ADD_ITEM("sum8", "simple", "Sum (8-bit)", "8-bit addition", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%04X", (uint16_t)(acc.sum & 0xFFFF));
    snprintf(decbuf, sizeof(decbuf), "%u", (uint16_t)(acc.sum & 0xFFFF));
    ADD_ITEM("sum16", "simple", "Sum (16-bit)", "16-bit addition", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%08X", (uint32_t)(acc.sum & 0xFFFFFFFF));
    snprintf(decbuf, sizeof(decbuf), "%u", (uint32_t)(acc.sum & 0xFFFFFFFF));
    ADD_ITEM("sum32", "simple", "Sum (32-bit)", "32-bit addition", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%02X", acc.xor8);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.xor8);
    ADD_ITEM("xor8", "simple", "XOR (8-bit)", "8-bit XOR checksum", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%08X", adler32);
    snprintf(decbuf, sizeof(decbuf), "%u", adler32);
    ADD_ITEM("adler32", "simple", "Adler-32", "poly=65521", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%04X", fletcher16);
    snprintf(decbuf, sizeof(decbuf), "%u", fletcher16);
    ADD_ITEM("fletcher16", "simple", "Fletcher-16", "16-bit checksum", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%08X", fletcher32);
    snprintf(decbuf, sizeof(decbuf), "%u", fletcher32);
    ADD_ITEM("fletcher32", "simple", "Fletcher-32", "32-bit checksum", hexbuf, decbuf);

    // CRC-8
    snprintf(hexbuf, sizeof(hexbuf), "%02X", acc.crc8_std);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc8_std);
    ADD_ITEM("crc8", "crc8", "CRC-8", "poly=0x07", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%02X", acc.crc8_cdma);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc8_cdma);
    ADD_ITEM("crc8cdma2000", "crc8", "CRC-8/CDMA2000", "poly=0x9B, init=0xFF", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%02X", acc.crc8_itu);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc8_itu);
    ADD_ITEM("crc8itu", "crc8", "CRC-8/ITU", "poly=0x07, xor=0x55", hexbuf, decbuf);

    // CRC-16
    snprintf(hexbuf, sizeof(hexbuf), "%04X", acc.crc16_arc);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc16_arc);
    ADD_ITEM("crc16arc", "crc16", "CRC-16/ARC", "poly=0x8005, refin", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%04X", acc.crc16_ccitt);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc16_ccitt);
    ADD_ITEM("crc16ccitt", "crc16", "CRC-16/CCITT-FALSE", "poly=0x1021, init=0xFFFF", hexbuf, decbuf);

    snprintf(hexbuf, sizeof(hexbuf), "%04X", acc.crc16_modbus);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.crc16_modbus);
    ADD_ITEM("crc16modbus", "crc16", "CRC-16/MODBUS", "poly=0x8005, init=0xFFFF, refin", hexbuf, decbuf);

    // CRC-32
    snprintf(hexbuf, sizeof(hexbuf), "%08X", crc32_final);
    snprintf(decbuf, sizeof(decbuf), "%u", crc32_final);
    ADD_ITEM("crc32", "crc32", "CRC-32 (IEEE 802.3)", "poly=0xEDB88320", hexbuf, decbuf);

    // CRC-64
    snprintf(hexbuf, sizeof(hexbuf), "%016llX", (unsigned long long)acc.crc64_ecma);
    snprintf(decbuf, sizeof(decbuf), "%llu", (unsigned long long)acc.crc64_ecma);
    ADD_ITEM("crc64", "crc64", "CRC-64/ECMA", "poly=0x42F0E1EBA9EA3693", hexbuf, decbuf);

    // FNV
    snprintf(hexbuf, sizeof(hexbuf), "%08X", acc.fnv1a_32);
    snprintf(decbuf, sizeof(decbuf), "%u", acc.fnv1a_32);
    ADD_ITEM("fnv1a32", "fnv", "FNV-1a (32-bit)", "Fowler-Noll-Vo hash", hexbuf, decbuf);

    // Hashes: MD5
    char md5_str[33];
    for (int i = 0; i < 16; i++) snprintf(md5_str + i * 2, 3, "%02x", md5_res[i]);
    ADD_ITEM("md5", "crypto", "MD5", "RFC 1321", md5_str, "-");

    // Hashes: SHA-1
    char sha1_str[41];
    for (int i = 0; i < 20; i++) snprintf(sha1_str + i * 2, 3, "%02x", sha1_res[i]);
    ADD_ITEM("sha1", "crypto", "SHA-1", "FIPS 180-1", sha1_str, "-");

    // Hashes: SHA-256
    char sha256_str[65];
    for (int i = 0; i < 32; i++) snprintf(sha256_str + i * 2, 3, "%02x", sha256_res[i]);
    ADD_ITEM("sha256", "crypto", "SHA-256", "FIPS 180-2", sha256_str, "-");

    #undef ADD_ITEM

    return list;
}

void checksums_free(checksum_list_t *list) {
    if (!list) return;
    free(list->items);
    free(list);
}
