#!/bin/sh

set -e
set -x

emcc -std=c23 -Os -msimd128 -sUSE_SDL=3 -o out/demo.html \
    iv2d.c iv2d_regions.c iv2d_vm.c stb_image_write.c prospero.c demo.c
open out/demo.html
