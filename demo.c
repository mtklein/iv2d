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

struct circle {
    float x,y,r;
};

static iv circle_edge(struct circle c, iv x, iv y) {
    return iv_sub(iv_add(iv_square(iv_sub(x, iv_(c.x))),
                         iv_square(iv_sub(y, iv_(c.y)))),
                  iv_(c.r*c.r));
}

static void render_circle(SDL_Renderer *renderer, struct circle c, int l, int t, int r, int b) {
    iv const edge = circle_edge(c, (iv){(float)l, (float)r}
                                 , (iv){(float)t, (float)b});

    if (edge.lo < 0 && edge.hi < 0) {
        // This rect has full coverage, drawn blue.
        SDL_FRect const rect = {(float)l, (float)t, (float)(r-l), (float)(b-t)};
        SDL_SetRenderDrawColor(renderer, 138,145,247,255);
        SDL_RenderFillRect    (renderer, &rect);
    }
    if (edge.lo < 0 && edge.hi >= 0) {
        int const x = (l+r)/2,
                  y = (t+b)/2;
        if (l == x && t == y) {
            // This pixel has partial coverage, drawn green.
            SDL_FRect const rect = {(float)l, (float)t, 1,1};
            SDL_SetRenderDrawColor(renderer, 97,175,75,255);
            SDL_RenderFillRect    (renderer, &rect);
        } else {
            // This rect has partial coverage, split and recurse.
            render_circle(renderer, c, l,t, x,y);
            render_circle(renderer, c, l,y, x,b);
            render_circle(renderer, c, x,t, r,y);
            render_circle(renderer, c, x,y, r,b);
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

    if (app->mode) {
        render_circle(app->renderer, c, 0,0,w,h);
    } else {
        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float const fx = (float)x,
                        fy = (float)y;
            iv const edge = circle_edge(c, (iv){fx,fx+1}
                                         , (iv){fy,fy+1});
            if (edge.lo < 0 && edge.hi < 0) {
                // Full coverage pixel, drawn grey.
                SDL_FRect const px = {fx,fy,1,1};
                SDL_SetRenderDrawColor(app->renderer, 155,155,155,255);
                SDL_RenderFillRect    (app->renderer, &px);
            }
            if (edge.lo < 0 && edge.hi >= 0) {
                // Partial coverage pixel, drawn red.
                SDL_FRect const px = {fx,fy,1,1};
                SDL_SetRenderDrawColor(app->renderer, 203,137,135,255);
                SDL_RenderFillRect    (app->renderer, &px);
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
