#define SDL_MAIN_USE_CALLBACKS 1
#include "iv.h"
#include "iv2d.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>

// TODO
//   - add a recurse-into-9-sub-parts coverage strategy
//   - add a scanlines coverage strategy
//   - how to best use vectorization?
//      - in subpixels, kind of obvious
//      - at larger scale?
//   - cover with other shapes?  triangles, quads?


struct app {
    struct iv2d_coverage_cb cov_cb;
    SDL_Window             *window;
    SDL_Renderer           *renderer;
    int                     full,partial;
    int                     quality,unused;
};

static void render_rect(struct iv2d_coverage_cb *cb, struct iv2d_rect rect, float cov) {
    struct app *app = (struct app*)cb;

    SDL_FRect const frect = {
        .x = (float)(rect.l         ),
        .y = (float)(rect.t         ),
        .w = (float)(rect.r - rect.l),
        .h = (float)(rect.b - rect.t),
    };

    *(cov == 1.0f ? &app->full : &app->partial) += 1;

    SDL_SetRenderDrawColorFloat(app->renderer, 0.5f, 0.5f, 0.5f, cov);
    SDL_RenderFillRect(app->renderer, &frect);
}

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    struct app *app = *ctx = calloc(1, sizeof *app);
    app->cov_cb.fn = render_rect;
    app->quality = argc > 1 ? atoi(argv[1]) : 1;

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
    free(app);
    SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void *ctx, SDL_Event *event) {
    struct app *app = ctx;
    switch (event->type) {
        default: break;

        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_KEY_DOWN:
            switch (event->key.key) {
                default: break;

                case SDLK_Q:
                case SDLK_ESCAPE:
                case SDLK_RETURN:
                    return SDL_APP_SUCCESS;

                case SDLK_MINUS:
                    app->quality--;
                    break;

                case SDLK_PLUS:
                case SDLK_EQUALS:
                    app->quality++;
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
    app->full = app->partial = 0;

    SDL_SetRenderDrawBlendMode (app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;
    struct iv2d_circle const c = iv2d_circle(cx,cy, 0.5f*(cx < cy ? cx : cy));

    // TODO: provide iv2d_bounds() to calculate this!
    struct iv2d_rect const bounds = {
        .l = (int)(c.x - c.r)    ,
        .t = (int)(c.y - c.r)    ,
        .r = (int)(c.x + c.r) + 1,
        .b = (int)(c.y + c.r) + 1,
    };

    double const start = now_us();
    {
        iv2d_cover(bounds, app->quality, &c.region, &app->cov_cb);
    }
    double const elapsed = now_us() - start;

    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderDebugTextFormat  (app->renderer, 0,4
                                             , "quality %d, %d full + %d partial, %.0fµs"
                                             , app->quality, app->full, app->partial, elapsed);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
