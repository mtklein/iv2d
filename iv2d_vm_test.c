#include "cleanup.h"
#include "iv2d_vm.h"
#include "test.h"
#include <stdlib.h>

int main(void) {
    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *region;
    {
        struct iv2d_builder *b = iv2d_builder();
        int const x = iv2d_x(b),
                  y = iv2d_y(b);
        region = iv2d_ret(b, iv2d_sub(b,x,y));
    }

    iv z = region->eval(region, (iv){{0,1,2,3}, {4,5,6,7}}
                              , (iv){{0,0,1,1}, {5,4,3,2}});
    expect(equiv(z.lo[0], -5));
    expect(equiv(z.hi[0],  4));

    expect(equiv(z.lo[1], -3));
    expect(equiv(z.hi[1],  5));

    expect(equiv(z.lo[2], -1));
    expect(equiv(z.hi[2],  5));

    expect(equiv(z.lo[3],  1));
    expect(equiv(z.hi[3],  6));
    return 0;
}
