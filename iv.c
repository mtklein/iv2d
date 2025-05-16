#include "iv.h"

iv iv_add(iv x, iv y) {
    return (iv){
        x.lo + y.lo,
        x.hi + y.hi,
    };
}
