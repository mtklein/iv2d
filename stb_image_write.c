#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stb_image_write.h"

static void write32be(stbi_write_func *func, void *ctx, uint32_t v) {
    uint8_t bytes[4] = { (uint8_t)(v>>24), (uint8_t)(v>>16), (uint8_t)(v>>8), (uint8_t)v };
    func(ctx, bytes, 4);
}

static uint32_t crc32(const void *data, size_t len, uint32_t crc) {
    static uint32_t table[256];
    static int init=0;
    if (!init) {
        for (uint32_t i=0;i<256;i++) {
            uint32_t c=i;
            for (int j=0;j<8;j++)
                c=c&1?0xedb88320^(c>>1):c>>1;
            table[i]=c;
        }
        init=1;
    }
    crc = ~crc;
    const uint8_t *p=data;
    for (size_t i=0;i<len;i++)
        crc = table[(crc^p[i])&0xff] ^ (crc>>8);
    return ~crc;
}

static uint32_t adler32(const uint8_t *data, size_t len) {
    uint32_t a=1,b=0;
    for (size_t i=0;i<len;i++) {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b<<16)|a;
}

static void write_chunk(stbi_write_func *func, void *ctx,
                        const char type[4], const unsigned char *data, size_t len) {
    write32be(func, ctx, (uint32_t)len);
    unsigned char t[4];
    memcpy(t, type, 4);
    func(ctx, t, 4);
    if (len) func(ctx, data, (int)len);
    uint32_t crc = crc32(type,4,0);
    crc = crc32(data,len,crc);
    write32be(func, ctx, crc);
}

int stbi_write_png_to_func(stbi_write_func *func, void *ctx,
                           int w, int h, int comp,
                           const void *data, int stride) {
    if (!func || !data || w<=0 || h<=0 || (comp!=3 && comp!=4)) return 0;

    static const unsigned char sig[8] = {137,80,78,71,13,10,26,10};
    func(ctx, sig, 8);

    uint8_t ihdr[13];
    ihdr[0]= (uint8_t)(w>>24); ihdr[1]=(uint8_t)(w>>16); ihdr[2]=(uint8_t)(w>>8); ihdr[3]=(uint8_t)w;
    ihdr[4]= (uint8_t)(h>>24); ihdr[5]=(uint8_t)(h>>16); ihdr[6]=(uint8_t)(h>>8); ihdr[7]=(uint8_t)h;
    ihdr[8]=8;
    ihdr[9]= comp==4 ? 6 : 2;
    ihdr[10]=0; ihdr[11]=0; ihdr[12]=0;
    write_chunk(func,ctx,"IHDR",ihdr,13);

    size_t row = (size_t)w * (size_t)comp + 1u;
    size_t img_size = row * (size_t)h;
    uint8_t *raw = malloc(img_size);
    if (!raw) return 0;
    for (int y=0; y < h; y++) {
        size_t off = (size_t)y * row;
        raw[off] = 0; // filter type 0
        memcpy(raw + off + 1, (const uint8_t*)data + (size_t)y * (size_t)stride,
               (size_t)w * (size_t)comp);
    }

    // Build deflate stream with no compression
    size_t deflate_cap = 2 + img_size + img_size/65535*5 + 6; // header + blocks + adler
    uint8_t *deflate = malloc(deflate_cap);
    if (!deflate) { free(raw); return 0; }
    size_t p=0;
    deflate[p++] = 0x78; deflate[p++] = 0x01; // deflate header
    size_t remaining = img_size;
    const uint8_t *src = raw;
    while (remaining) {
        uint16_t chunk = remaining > 65535 ? 65535 : (uint16_t)remaining;
        remaining -= chunk;
        uint8_t final = remaining==0; // 1 if last block
        deflate[p++] = final;
        deflate[p++] = chunk & 0xff; deflate[p++] = chunk >> 8;
        deflate[p++] = (~chunk) & 0xff; deflate[p++] = (~chunk)>>8;
        memcpy(deflate+p, src, chunk);
        p += chunk; src += chunk;
    }
    uint32_t ad = adler32(raw,img_size);
    deflate[p++] = (uint8_t)(ad>>24); deflate[p++] = (uint8_t)(ad>>16);
    deflate[p++] = (uint8_t)(ad>>8);  deflate[p++] = (uint8_t)ad;

    write_chunk(func,ctx,"IDAT",deflate,p);
    free(deflate);
    free(raw);

    write_chunk(func,ctx,"IEND",NULL,0);
    return 1;
}
