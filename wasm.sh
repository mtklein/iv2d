#!/bin/sh

set -e
set -x

emcc -Os -msimd128 -sUSE_SDL=3 iv2d.c iv2d_regions.c iv2d_vm.c stb_image_write.c demo.c \
    -o out/demo.html
open out/demo.html
