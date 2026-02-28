#include "slides.h"
#include "iv2d.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static void noop(void *ctx, float l, float t, float r, float b, float cov) {
    (void)ctx; (void)l; (void)t; (void)r; (void)b; (void)cov;
}

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Usage: bench [quality] [WxH]
int main(int argc, char *argv[]) {
    int quality = 0, w = 800, h = 600;

    for (int i = 1; i < argc; i++) {
        int W,H;
        if (2 == sscanf(argv[i], "%dx%d", &W, &H)) { w = W; h = H; continue; }

        int n;
        if (1 == sscanf(argv[i], "%d", &n)) { quality = n; continue; }
    }

    struct slides slides;
    slides_init(&slides, w, h, 0);

    for (int i = 0; i < slides.count; i++) {
        double const start = now();
        iv2d_cover(slides.slide[i].region, 0,0,w,h, quality, noop,NULL);
        double const elapsed = now() - start;
        printf("%16s %10.2f\u00b5s\n", slides.slide[i].name, 1e6 * elapsed);
    }

    slides_fini(&slides);
    return 0;
}
