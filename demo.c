#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdlib.h>

// TODO
//   - implement some sort of partial coverage in both modes
//   - factor apart recursion/coverage strategy from edge function, should be mix'n'match
//   - add a recurse-into-9-sub-parts strategy
//   - how to best use vectorization?
//   - cover with triangles, quads?

struct coverage {
    SDL_FRect *part,*full;
    float     *part_cov;
    int        parts, part_cap;
    int        fulls, full_cap;
};

struct app {
    SDL_Window     *window;
    SDL_Renderer   *renderer;
    int             mode,unused;
    struct coverage cov;
};

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    struct app *app = *ctx = calloc(1, sizeof *app);
    app->mode = argc > 1 ? atoi(argv[1]) : 0;

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

static inline iv iv_(float x) {
    return (iv){x,x};
}

struct circle {
    float x,y,r;
};

static iv circle_edge(struct circle c, iv x, iv y) {
    return iv_sub(iv_add(iv_square(iv_sub(x, iv_(c.x))),
                         iv_square(iv_sub(y, iv_(c.y)))),
                  iv_(c.r*c.r));
}

static void push_coverage(struct coverage *cov, int l, int t, int r, int b, float v) {
    SDL_FRect const rect = { (float)l, (float)t, (float)(r-l), (float)(b-t) };
    if (v == 1.0f) {
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
        cov->part_cov[cov->parts  ] = v;
        cov->part    [cov->parts++] = rect;
    }
}

static void cover_circle(struct coverage *cov, struct circle c, int l, int t, int r, int b) {
    if (l < r && t < b) {
        iv const edge = circle_edge(c, (iv){(float)l, (float)r}
                                     , (iv){(float)t, (float)b});

        if (edge.lo < 0 && edge.hi < 0) {
            push_coverage(cov, l,t,r,b, 1.0f);
        }
        if (edge.lo < 0 && edge.hi >= 0) {
            int const x = (l+r)/2,
                      y = (t+b)/2;
            if (l == x && t == y) {
                push_coverage(cov, l,t,r,b, 0.5f/*TODO*/);
            } else {
                // This rect has partial coverage, split and recurse.
                cover_circle(cov, c, l,t, x,y);
                cover_circle(cov, c, l,y, x,b);
                cover_circle(cov, c, x,t, r,y);
                cover_circle(cov, c, x,y, r,b);
            }
        }
    }
}


SDL_AppResult SDL_AppIterate(void *ctx) {
    struct app *app = ctx;

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;
    struct circle const c = { cx,cy, 0.5f*fminf(cx,cy) };

    SDL_SetRenderDrawColor(app->renderer, 255,255,255,255);
    SDL_RenderClear       (app->renderer                 );

    uint64_t const freq = SDL_GetPerformanceFrequency(),
                  start = SDL_GetPerformanceCounter();

    int const l = (int)(c.x - c.r)    ,
              t = (int)(c.y - c.r)    ,
              r = (int)(c.x + c.r) + 1,
              b = (int)(c.y + c.r) + 1;

    struct coverage *cov = &app->cov;
    cov->fulls = cov->parts = 0;

    struct { uint8_t r,g,b; } full, part;
    if (app->mode) {
        full.r = 138; full.g = 145; full.b = 247;
        part.r =  97; part.g = 175; part.b =  75;
        cover_circle(cov, c, l,t,r,b);
    } else {
        full.r = 155; full.g = 155; full.b = 155;
        part.r = 203; part.g = 137; part.b = 135;
        for (int y = t; y < b; y++)
        for (int x = l; x < r; x++) {
            float const fx = (float)x,
                        fy = (float)y;
            iv const edge = circle_edge(c, (iv){fx,fx+1}
                                         , (iv){fy,fy+1});
            if (edge.lo < 0 && edge.hi < 0) {
                push_coverage(cov, x,y,x+1,y+1, 1.0f);
            }
            if (edge.lo < 0 && edge.hi >= 0) {
                push_coverage(cov, x,y,x+1,y+1, 0.5f/*TODO*/);
            }
        }
    }
    SDL_SetRenderDrawColor(app->renderer, full.r, full.g, full.b, 255);
    SDL_RenderFillRects   (app->renderer, cov->full, cov->fulls);
    SDL_SetRenderDrawColor(app->renderer, part.r, part.g, part.b, 255);
    SDL_RenderFillRects   (app->renderer, cov->part, cov->parts);

    uint64_t const elapsed = SDL_GetPerformanceCounter() - start;
    SDL_SetRenderDrawColor   (app->renderer, 0,0,0,255);
    SDL_RenderDebugTextFormat(app->renderer, 0,4,
                              "%d %d full %d part %.0fµs", app->mode, cov->fulls, cov->parts
                                                         , (double)elapsed * 1e6 / (double)freq);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
