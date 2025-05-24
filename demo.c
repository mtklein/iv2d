#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int           mode,unused;

    SDL_FRect *part,*full;
    float     *part_cov;
    int        parts, part_cap;
    int        fulls, full_cap;
} App;

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    App *app = *ctx = calloc(1, sizeof *app);
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
    App *app = ctx;
    (void)res;

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    free(app->full);
    free(app->part);
    free(app->part_cov);
    free(app);
    SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void *ctx, SDL_Event *event) {
    App *app = ctx;
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

static void push_coverage(App *app, int l, int t, int r, int b, float cov) {
    SDL_FRect const rect = { (float)l, (float)t, (float)(r-l), (float)(b-t) };
    if (cov == 1.0f) {
        if (app->fulls == app->full_cap) {
            app->full_cap = app->full_cap ? 2*app->full_cap : 1;
            app->full = realloc(app->full, (size_t)app->full_cap * sizeof *app->full);
        }
        app->full[app->fulls++] = rect;
    } else {
        if (app->parts == app->part_cap) {
            app->part_cap = app->part_cap ? 2*app->part_cap : 1;
            app->part_cov = realloc(app->part_cov, (size_t)app->part_cap * sizeof *app->part_cov);
            app->part     = realloc(app->part    , (size_t)app->part_cap * sizeof *app->part    );
        }
        app->part_cov[app->parts  ] = cov;
        app->part    [app->parts++] = rect;
    }
}

static void cover_circle(App *app, struct circle c, int l, int t, int r, int b) {
    iv const edge = circle_edge(c, (iv){(float)l, (float)r}
                                 , (iv){(float)t, (float)b});

    if (edge.lo < 0 && edge.hi < 0) {
        push_coverage(app, l,t,r,b, 1.0f);
    }
    if (edge.lo < 0 && edge.hi >= 0) {
        int const x = (l+r)/2,
                  y = (t+b)/2;
        if (l == x && t == y) {
            push_coverage(app, l,t,r,b, 0.5f/*TODO*/);
        } else {
            // This rect has partial coverage, split and recurse.
            cover_circle(app, c, l,t, x,y);
            cover_circle(app, c, l,y, x,b);
            cover_circle(app, c, x,t, r,y);
            cover_circle(app, c, x,y, r,b);
        }
    }
}


SDL_AppResult SDL_AppIterate(void *ctx) {
    App *app = ctx;

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;
    struct circle const c = { cx,cy, 0.5f*fminf(cx,cy) };

    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    uint64_t const freq = SDL_GetPerformanceFrequency(),
                  start = SDL_GetPerformanceCounter();

    struct { uint8_t r,g,b; } full, part;
    app->fulls = app->parts = 0;
    if (app->mode) {
        full.r = 138; full.g = 145; full.b = 247;
        part.r =  97; part.g = 175; part.b =  75;
        cover_circle(app, c, 0,0,w,h);
    } else {
        full.r = 155; full.g = 155; full.b = 155;
        part.r = 203; part.g = 137; part.b = 135;
        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float const fx = (float)x,
                        fy = (float)y;
            iv const edge = circle_edge(c, (iv){fx,fx+1}
                                         , (iv){fy,fy+1});
            if (edge.lo < 0 && edge.hi < 0) {
                push_coverage(app, x,y,x+1,y+1, 1.0f);
            }
            if (edge.lo < 0 && edge.hi >= 0) {
                push_coverage(app, x,y,x+1,y+1, 0.5f/*TODO*/);
            }
        }
    }
    SDL_SetRenderDrawColor(app->renderer, full.r, full.g, full.b, 255);
    SDL_RenderFillRects   (app->renderer, app->full, app->fulls);
    SDL_SetRenderDrawColor(app->renderer, part.r, part.g, part.b, 255);
    SDL_RenderFillRects   (app->renderer, app->part, app->parts);

    uint64_t const elapsed = SDL_GetPerformanceCounter() - start;
    char txt[128];
    snprintf(txt, sizeof txt, "%d %d full %d part %.0fµs",
             app->mode, app->fulls, app->parts, (double)elapsed * 1e6 / (double)freq);
    SDL_SetRenderDrawColor(app->renderer, 0,0,0,255);
    SDL_RenderDebugText   (app->renderer, 0,4, txt);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
