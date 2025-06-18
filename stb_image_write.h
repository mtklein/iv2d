#pragma once
#include <stdint.h>
typedef void stbi_write_func(void*, void*, int);
int stbi_write_png_to_func(stbi_write_func*, void*, int,int,int,const void*, int);
