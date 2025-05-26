#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include "iv2d.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdlib.h>

// TODO
//   - implement some sort of partial coverage in both modes
//   - add a recurse-into-9-sub-parts strategy
//   - how to best use vectorization?
//   - cover with triangles, quads?

struct coverage_for_SDL {
    struct iv2d_yield_coverage yield;

    SDL_FRect *part,*full;
    float     *part_cov;
    int        parts, part_cap;
    int        fulls, full_cap;
};

static void yield_coverage_for_SDL(struct iv2d_yield_coverage *yield,
                                   struct iv2d_rect bounds, float c) {
    struct coverage_for_SDL *cov = (struct coverage_for_SDL*)yield;

    SDL_FRect const rect = {
        .x = (float)bounds.l,
        .y = (float)bounds.t,
        .w = (float)(bounds.r-bounds.l),
        .h = (float)(bounds.b-bounds.t),
    };
    if (c == 1.0f) {
        if (cov->fulls == cov->full_cap) {
            cov->full_cap = cov->full_cap ? 2*cov->full_cap : 1;
            cov->full = realloc(cov->full, (size_t)cov->full_cap * sizeof *cov->full);
        }
        cov->full[cov->fulls++] = rect;
    } else {
        if (cov->parts == cov->part_cap) {
            cov->part_cap = cov->part_cap ? 2*cov->part_cap : 1;
            cov->part_cov = realloc(cov->part_cov, (size_t)cov->part_cap * sizeof *cov->part_cov);
            cov->part     = realloc(cov->part    , (size_t)cov->part_cap * sizeof *cov->part    );
        }
        cov->part_cov[cov->parts  ] = c;
        cov->part    [cov->parts++] = rect;
    }
}

struct app {
    SDL_Window             *window;
    SDL_Renderer           *renderer;
    int                     mode,unused;
    struct coverage_for_SDL cov;
};

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    struct app *app = *ctx = calloc(1, sizeof *app);
    app->mode = argc > 1 ? atoi(argv[1]) : 0;
    app->cov.yield.fn = yield_coverage_for_SDL;

    if (!SDL_CreateWindowAndRenderer("iv2d demo", 800, 600, SDL_WINDOW_RESIZABLE,
                                     &app->window, &app->renderer)) {
        free(app);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(app->window, 0,0);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *ctx, SDL_AppResult res) {
    struct app *app = ctx;
    (void)res;

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    free(app->cov.full);
    free(app->cov.part);
    free(app->cov.part_cov);
    free(app);
    SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void *ctx, SDL_Event *event) {
    struct app *app = ctx;
    switch (event->type) {
        default: break;
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;

        case SDL_EVENT_KEY_DOWN:
            switch (event->key.key) {
                default: break;
                case SDLK_ESCAPE:
                case SDLK_RETURN: return SDL_APP_SUCCESS;

                case SDLK_M:
                    app->mode ^= 1;
                    break;
            }
            break;
    }
    return SDL_APP_CONTINUE;
}

static double now_us(void) {
    static double scale = -1;
    if (scale < 0) {
        uint64_t freq = SDL_GetPerformanceFrequency();
        scale = 1e6 / (double)freq;
    }
    uint64_t now = SDL_GetPerformanceCounter();
    return (double)now * scale;
}

SDL_AppResult SDL_AppIterate(void *ctx) {
    struct app *app = ctx;

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;

    struct iv2d_circle const c = iv2d_circle(cx,cy, 0.5f*fminf(cx,cy));

    SDL_SetRenderDrawColor(app->renderer, 255,255,255,255);
    SDL_RenderClear       (app->renderer                 );

    struct iv2d_rect const bounds = {
        .l = (int)(c.x - c.r)    ,
        .t = (int)(c.y - c.r)    ,
        .r = (int)(c.x + c.r) + 1,
        .b = (int)(c.y + c.r) + 1,
    };

    struct coverage_for_SDL *cov = &app->cov;
    cov->fulls = cov->parts = 0;

    struct { uint8_t r,g,b; } full, part;

    double const start_us = now_us();
    if (app->mode) {
        full.r = 138; full.g = 145; full.b = 247;
        part.r =  97; part.g = 175; part.b =  75;
        iv2d_cover(bounds, &c.edge, &cov->yield);
    } else {
        full.r = 155; full.g = 155; full.b = 155;
        part.r = 203; part.g = 137; part.b = 135;
        for (int y = bounds.t; y < bounds.b; y++)
        for (int x = bounds.l; x < bounds.r; x++) {
            struct iv2d_rect const pixel = {x,y,x+1,y+1};
            float const fx = (float)x,
                        fy = (float)y;
            iv const e = c.edge.fn(&c.edge, (iv){fx,fx+1}
                                          , (iv){fy,fy+1});
            if (e.lo < 0 && e.hi < 0) {
                cov->yield.fn(&cov->yield, pixel, 1.0f);
            }
            if (e.lo < 0 && e.hi >= 0) {
                cov->yield.fn(&cov->yield, pixel, 0.5f/*TODO*/);
            }
        }
    }
    double const cover_us = now_us() - start_us;

    SDL_SetRenderDrawColor   (app->renderer, full.r, full.g, full.b, 255);
    SDL_RenderFillRects      (app->renderer, cov->full, cov->fulls);
    SDL_SetRenderDrawColor   (app->renderer, part.r, part.g, part.b, 255);
    SDL_RenderFillRects      (app->renderer, cov->part, cov->parts);
    double const issue_us = now_us() - start_us;

    SDL_SetRenderDrawColor   (app->renderer, 0,0,0,255);
    SDL_RenderDebugTextFormat(app->renderer, 0,4,
                              "mode %d, %d full + %d partial, %.0f + %.0f = %.0fµs",
                              app->mode, cov->fulls, cov->parts,
                              cover_us, issue_us - cover_us, issue_us);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
