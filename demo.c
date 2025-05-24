#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int           mode,unused;
} App;

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    App *app = *ctx = SDL_calloc(1, sizeof *app);

    if (!SDL_CreateWindowAndRenderer("iv2d demo", 800, 600, SDL_WINDOW_RESIZABLE,
                                     &app->window, &app->renderer)) {
        SDL_free(app);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *ctx, SDL_AppResult res) {
    App *app = ctx;
    (void)res;

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    SDL_free(app);
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

static iv circle_edge(float cx, float cy, float r, iv x, iv y) {
    return iv_sub(iv_add(iv_square(iv_sub(x, iv_(cx))),
                         iv_square(iv_sub(y, iv_(cy)))),
                  iv_(r*r));
}

static void render_circle(SDL_Renderer *rend, float cx, float cy, float r, iv x, iv y) {
    iv const circle = circle_edge(cx,cy,r,x,y);

    if (circle.lo < 0 && circle.hi < 0) {
        SDL_FRect const rect = {x.lo,y.lo, x.hi-x.lo,y.hi-y.lo};
        SDL_SetRenderDrawColorFloat(rend, 0,0,1,1);
        SDL_RenderFillRect         (rend, &rect);
    }
    /*
    if (circle.lo < 0 && circle.hi >= 0) {
        float const xm = (x.lo + x.hi) / 2,
                    ym = (y.lo + y.hi) / 2;
        if (x.lo < xm && y.lo < ym) { render_circle(rend,cx,cy,r, (iv){x.lo,xm}, (iv){y.lo,ym}); }
        if (x.lo < xm && ym < y.hi) { render_circle(rend,cx,cy,r, (iv){x.lo,xm}, (iv){ym,y.hi}); }
        if (xm < x.hi && y.lo < ym) { render_circle(rend,cx,cy,r, (iv){xm,x.hi}, (iv){y.lo,ym}); }
        if (xm < x.hi && ym < y.hi) { render_circle(rend,cx,cy,r, (iv){xm,x.hi}, (iv){ym,y.hi}); }
    }
    */
}


SDL_AppResult SDL_AppIterate(void *ctx) {
    App *app = ctx;

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h,
                r  = 0.5f * fminf(cx,cy);

    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    uint64_t const freq = SDL_GetPerformanceFrequency(),
                  start = SDL_GetPerformanceCounter();

    if (app->mode) {
        render_circle(app->renderer, cx,cy,r, (iv){0,(float)w}, (iv){0,(float)h});
    } else {
        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float const fx = (float)x,
                        fy = (float)y;
            iv const circle = circle_edge(cx,cy,r, (iv){fx,fx+1}
                                                 , (iv){fy,fy+1});
            if (circle.lo < 0 && circle.hi < 0) {
                SDL_FRect const px = {fx,fy,1,1};
                SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
                SDL_RenderFillRect         (app->renderer, &px);
            }
            if (circle.lo < 0 && circle.hi >= 0) {
                SDL_FRect const px = {fx,fy,1,1};
                SDL_SetRenderDrawColorFloat(app->renderer, 1,0,0,1);
                SDL_RenderFillRect         (app->renderer, &px);
            }
        }
    }
    uint64_t const elapsed = SDL_GetPerformanceCounter() - start;

    char txt[128];
    snprintf(txt, sizeof txt, "%d %.0fµs", app->mode, (double)elapsed * 1e6 / (double)freq);
    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderDebugText        (app->renderer, 0,4,txt);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
