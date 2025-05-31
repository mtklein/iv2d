#define SDL_MAIN_USE_CALLBACKS 1
#include "iv2d.h"
#include "stb/stb_image_write.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>

#define len(x) (int)( sizeof(x) / sizeof(x[0]) )

static void write_to_stdout(void *ctx, void *buf, int len) {
    (void)ctx;
    fwrite(buf, 1, (size_t)len, stdout);
}

// TODO
//   - add a recurse-into-9-sub-parts coverage strategy
//   - add a scanlines coverage strategy
//   - how to best use vectorization in iv2d_cover()?
//   - cover with other shapes?  triangles, quads?

struct quad {
    struct { float x,y; SDL_FColor c; } vertex[6];
};

struct app {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    struct quad  *quad;
    int           quads, quad_cap, full;
    int           quality, slide;
    bool          draw_bounds, write_png, paddingA[2];

    uint64_t frametime[32];
    int      frametimes, paddingB;
};

static void reset_frametimes(struct app *app) {
    for (int i = 0; i < len(app->frametime); i++) {
        app->frametime[i] = 0;
    }
}

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
            case 'p': app->write_png   ^= 1; break;
        }
    }
    return false;
}

static void queue_rect(void *arg, float l, float t, float r, float b, float cov) {
    struct app *app = (struct app*)arg;
    app->full += cov == 1.0f;

    if (app->quad_cap == app->quads) {
        app->quad_cap = app->quad_cap ? 2 * app->quad_cap : 1;
        app->quad = SDL_realloc(app->quad, (size_t)app->quad_cap * sizeof *app->quad);
    }
    SDL_FColor const c = {0.5f, 0.5f, 0.5f, cov};
    app->quad[app->quads++] = (struct quad) {{
        {l,t,c}, {r,t,c}, {l,b,c},
        {r,t,c}, {r,b,c}, {l,b,c},
    }};
}

SDL_AppResult SDL_AppInit(void **ctx, int argc, char *argv[]) {
    struct app *app = *ctx = SDL_calloc(1, sizeof *app);

    int w=800, h=600;
    for (int i = 1; i < argc; i++) {
        if (2 == sscanf(argv[i], "%dx%d", &w, &h)) {
            continue;
        }
        if (handle_keys(app, argv[i])) {
            return SDL_APP_SUCCESS;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("iv2d demo", w,h,
                                     SDL_WINDOW_RESIZABLE|(app->write_png ? SDL_WINDOW_HIDDEN : 0),
                                     &app->window, &app->renderer)) {
        SDL_free(app);
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
    SDL_free(app->quad);
    SDL_free(app);
    SDL_Quit();
}

SDL_AppResult SDL_AppEvent(void *ctx, SDL_Event *event) {
    struct app *app = ctx;
    switch (event->type) {
        default: break;

        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_KEY_DOWN:
            reset_frametimes(app);
            if (handle_keys(app, (char const[]){(char)event->key.key,0})) {
                return SDL_APP_SUCCESS;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            reset_frametimes(app);
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *ctx) {
    struct app *app = ctx;
    app->quads = app->full = 0;

    SDL_SetRenderDrawBlendMode (app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h;

    struct iv2d_circle const centered = {cx,cy, 0.5f*fminf(cx,cy)},
                                fixed = {300,200,100};

    struct iv2d_binop const scene = {
        iv2d_circle, &centered,
        iv2d_circle, &fixed,
    };

    struct {
        iv2d_region *region;
        char const  *name;
    } slides[] = {
        {iv2d_union       , "union"       },
        {iv2d_intersection, "intersection"},
        {iv2d_difference  , "difference"  },
    };
    int slide = app->slide;
    if (slide <             0) { slide =             0; }
    if (slide > len(slides)-1) { slide = len(slides)-1; }

    uint64_t const start = SDL_GetPerformanceCounter();
    {
        iv2d_cover(slides[slide].region, &scene, 0,0,w,h, app->quality, queue_rect,app);
    }
    app->frametime[app->frametimes++ % len(app->frametime)] = SDL_GetPerformanceCounter() - start;

    uint64_t avg_frametime;
    {
        uint64_t sum = 0;
        int  nonzero = 0;
        for (int i = 0; i < len(app->frametime); i++) {
            sum     += app->frametime[i];
            nonzero += app->frametime[i] > 0;
        }
        avg_frametime = sum / (unsigned)nonzero;
    }

    SDL_RenderGeometryRaw(app->renderer, NULL
                                       , &app->quad->vertex->x, sizeof *app->quad->vertex
                                       , &app->quad->vertex->c, sizeof *app->quad->vertex
                                       , NULL, 0
                                       , len(app->quad->vertex) * app->quads
                                       , NULL, 0, 0);
    if (app->draw_bounds) {
        float l = +1.0f/0.0f,
              t = +1.0f/0.0f,
              r = -1.0f/0.0f,
              b = -1.0f/0.0f;
        for (int i = 0; i <     app->quads        ; i++)
        for (int j = 0; j < len(app->quad->vertex); j++) {
            l = fminf(l, app->quad[i].vertex[j].x);
            t = fminf(t, app->quad[i].vertex[j].y);
            r = fmaxf(r, app->quad[i].vertex[j].x);
            b = fmaxf(b, app->quad[i].vertex[j].y);
        }
        SDL_FRect const bounds = {l,t,r-l,b-t};
        SDL_SetRenderDrawColorFloat(app->renderer, 1,0,0,0.125);
        SDL_RenderRect             (app->renderer, &bounds);
    }

    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderDebugTextFormat  (app->renderer, 4,4,
            "%s (%d), %dx%d, quality %d, %d full + %d partial, %lluµs",
            slides[slide].name, slide, w,h, app->quality, app->full, app->quads - app->full,
            app->write_png ? 0 : 1000000 * avg_frametime / SDL_GetPerformanceFrequency());

    if (app->write_png) {
        SDL_Surface *surf = SDL_RenderReadPixels(app->renderer, NULL),
                    *rgba = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surf);

        stbi_write_png_to_func(write_to_stdout,NULL,
                               rgba->w, rgba->h, 4, rgba->pixels, rgba->pitch);
        SDL_DestroySurface(rgba);
        return SDL_APP_SUCCESS;
    }

    SDL_RenderPresent(app->renderer);
    return SDL_APP_CONTINUE;
}
