#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
} App;

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    App *app = *ctx = SDL_malloc(sizeof *app);

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
    (void)ctx;
    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        default:             break;
    }
    return SDL_APP_CONTINUE;
}

static iv cover_circle(iv x, iv y, float cx, float cy, float r) {
    iv const dx = iv_sub(x, (iv){cx,cx}),
             dy = iv_sub(y, (iv){cy,cy});
    return iv_sub(iv_add(iv_mul(dx,dx),
                         iv_mul(dy,dy)),
                  (iv){r*r,r*r});
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
    for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
        float const fx = (float)x,
                    fy = (float)y;
        iv const circle = cover_circle((iv){fx,fx+1},
                                       (iv){fy,fy+1},
                                       cx,cy,r);
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
    uint64_t const elapsed = SDL_GetPerformanceCounter() - start;

    char txt[128];
    snprintf(txt, sizeof txt, "%.0fµs", (double)elapsed * 1e6 / (double)freq);
    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderDebugText        (app->renderer, 0,4, txt);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
