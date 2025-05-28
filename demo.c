#define SDL_MAIN_USE_CALLBACKS 1
#include "iv2d.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>

#define len(x) (int)( sizeof(x) / sizeof(x[0]) )

// TODO
//   - add a recurse-into-9-sub-parts coverage strategy
//   - add a scanlines coverage strategy
//   - how to best use vectorization?
//      - in subpixels, kind of obvious
//      - at larger scale?
//   - cover with other shapes?  triangles, quads?

struct quad {
    struct { float x,y; SDL_FColor c; } vertex[6];
};

struct app {
    struct iv2d_coverage_cb cov_cb;

    SDL_Window   *window;
    SDL_Renderer *renderer;
    struct quad  *quad;
    int           quads, quad_cap;
    int           full, partial;
    SDL_FRect     bounds;
    int           quality, draw_bounds, slide, padding;
};

static _Bool handle_keys(struct app *app, char const *key) {
    while (*key) {
        switch (*key++) {
            default: break;

            case 'q':
            case SDLK_RETURN:
            case SDLK_ESCAPE: return true;

            case '-': app->quality--; break;
            case '+':
            case '=': app->quality++; break;

            case '[': app->slide--; break;
            case ']': app->slide++; break;

            case 'b': app->draw_bounds ^= 1; break;
        }
    }
    return false;
}

static void queue_rect(struct iv2d_coverage_cb *cb, struct iv2d_rect rect, float cov) {
    struct app *app = (struct app*)cb;
    *(cov == 1.0f ? &app->full : &app->partial) += 1;

    if (app->quad_cap == app->quads) {
        app->quad_cap = app->quad_cap ? 2 * app->quad_cap : 1;
        app->quad = realloc(app->quad, (size_t)app->quad_cap * sizeof *app->quad);
    }

    float const l = (float)rect.l,
                t = (float)rect.t,
                r = (float)rect.r,
                b = (float)rect.b;
    SDL_FColor const c = {0.5f, 0.5f, 0.5f, cov};
    app->quad[app->quads++] = (struct quad) {{
        {l,t,c}, {r,t,c}, {l,b,c},
        {r,t,c}, {r,b,c}, {l,b,c},
    }};

    if (app->draw_bounds) {
        SDL_FRect const frect = {
            .x = (float)(rect.l         ),
            .y = (float)(rect.t         ),
            .w = (float)(rect.r - rect.l),
            .h = (float)(rect.b - rect.t),
        };
        SDL_GetRectUnionFloat(&frect, &app->bounds, &app->bounds);
    }
}

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }

    struct app *app = *ctx = calloc(1, sizeof *app);
    app->cov_cb.fn = queue_rect;

    if (!SDL_CreateWindowAndRenderer("iv2d demo", 800, 600, SDL_WINDOW_RESIZABLE,
                                     &app->window, &app->renderer)) {
        free(app);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(app->window, 0,0);

    return (argc > 1 && handle_keys(app, argv[1])) ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *ctx, SDL_AppResult res) {
    struct app *app = ctx;
    (void)res;

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    free(app->quad);
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
            if (handle_keys(app, (char const[]){(char)event->key.key,0})) {
                return SDL_APP_SUCCESS;
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
    app->full = app->partial = app->quads = 0;
    app->bounds = (SDL_FRect){0,0,-1,-1};

    SDL_SetRenderDrawBlendMode (app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;
    struct iv2d_circle const centered = iv2d_circle(cx,cy, 0.5f*(cx < cy ? cx : cy)),
                                fixed = iv2d_circle(300,200,100);

    struct iv2d_union        const u = iv2d_union       (&centered.region, &fixed.region);
    struct iv2d_intersection const i = iv2d_intersection(&centered.region, &fixed.region);
    struct iv2d_difference   const d = iv2d_difference  (&centered.region, &fixed.region);

    struct {
        struct iv2d_region const *region;
        char               const *name;
    } slides[] = {
        {&u.region, "union"},
        {&i.region, "intersection"},
        {&d.region, "difference"},
    };

    int slide = app->slide;
    if (slide <             0) { slide =             0; }
    if (slide > len(slides)-1) { slide = len(slides)-1; }


    double const start = now_us();
    {
        iv2d_cover((struct iv2d_rect){0,0,w,h}, app->quality, slides[slide].region, &app->cov_cb);
    }
    double const elapsed = now_us() - start;

    SDL_RenderGeometryRaw(app->renderer, NULL
                                       , &app->quad->vertex->x, sizeof *app->quad->vertex
                                       , &app->quad->vertex->c, sizeof *app->quad->vertex
                                       , NULL, 0
                                       , 6 * app->quads
                                       , NULL, 0, 0);
    if (app->draw_bounds) {
        SDL_SetRenderDrawColorFloat(app->renderer, 1,0,0,0.125);
        SDL_RenderRect             (app->renderer, &app->bounds);
    }
    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderDebugTextFormat  (app->renderer,
            4,4, "%s (%d), quality %d, %d full + %d partial, %.0fµs"
               , slides[slide].name, slide, app->quality, app->full, app->partial, elapsed);

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
