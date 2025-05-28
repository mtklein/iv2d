#pragma once

typedef struct {
    float lo,hi;
} iv;

iv iv_add(iv, iv);
iv iv_sub(iv, iv);
iv iv_mul(iv, iv);
iv iv_min(iv, iv);
iv iv_max(iv, iv);

iv iv_neg   (iv);
iv iv_sqrt  (iv);
iv iv_square(iv);
