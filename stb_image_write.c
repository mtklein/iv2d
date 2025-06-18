#include <stdint.h>
#include "stb_image_write.h"

int stbi_write_png_to_func(stbi_write_func *func, void *ctx,
                           int w, int h, int comp,
                           const void *data, int stride) {
    (void)w;(void)h;(void)comp;(void)data;(void)stride;
    // stub: write nothing but succeed
    (void)func;(void)ctx;
    return 1;
}
