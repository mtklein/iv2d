#ifndef STB_IMAGE_WRITE_H
#define STB_IMAGE_WRITE_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void stbi_write_func(void *context, const void *data, int size);

int stbi_write_png_to_func(stbi_write_func *func, void *context,
                           int w, int h, int comp,
                           const void *data, int stride_in_bytes);

#ifdef __cplusplus
}
#endif
#endif
