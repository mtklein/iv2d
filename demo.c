#define SDL_MAIN_USE_CALLBACKS 1
#include "cleanup.h"
#include "iv2d.h"
#include "iv2d_regions.h"
#include "iv2d_vm.h"
#include "len.h"
#include "slide.h"
#include "stb/stb_image_write.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <math.h>
#include <stdio.h>

static int wrap(int x, int n) {
    return ((x % n) + n) % n;
}

static void write_to_stdout(void *ctx, void *buf, int len) {
    (void)ctx;
    fwrite(buf, 1, (size_t)len, stdout);
}

// TODO
//   - try using stbtt_FlattenCurves() to make some piecewise capsules
//   - tiger

struct quad {
    struct { float x,y; SDL_FColor c; } vertex[6];
};

struct app {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    struct quad  *quad;
    int           quads, quad_cap, full;

    int quality, slide;
    int draw_bounds :  1;
    int write_png   :  1;
    int animate     :  1;
    int stroke      :  1;
    int paddingA    : 28;

    double frametime[32];
    int    next_frametime, paddingB;

    double time,start_time;
};

static void reset_frametimes(struct app *app) {
    for (int i = 0; i < len(app->frametime); i++) {
        app->frametime[i] = 0;
    }
}

static double now(void) {
    static double to_sec = 0;
    if (to_sec <= 0) {
        to_sec = 1 / (double)(uint64_t)SDL_GetPerformanceFrequency();
    }
    return (double)(uint64_t)SDL_GetPerformanceCounter() * to_sec;
}

static _Bool handle_keys(struct app *app, char const *key) {
    for (; *key; key++) {
        switch (*key) {
            default: break;

            case 'q':
            case SDLK_RETURN:
            case SDLK_ESCAPE: return true;

            case '-': if (--app->quality < 0) app->quality=0; break;
            case '+':
            case '=': app->quality++; break;

            case '[': app->slide--; break;
            case ']': app->slide++; break;

            case 'a': app->animate     ^= 1; break;
            case 'b': app->draw_bounds ^= 1; break;
            case 'p': app->write_png   ^= 1; break;
            case 's': app->stroke      ^= 1; break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': app->slide = *key-'0'; break;
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
    app->start_time = now();

    int w=800, h=600;
    for (int i = 1; i < argc; i++) {
        int W,H;
        if (2 == sscanf(argv[i], "%dx%d", &W, &H)) {
            w = W;
            h = H;
            continue;
        }
        if (handle_keys(app, argv[i])) {
            return SDL_APP_SUCCESS;
        }
    }

    if (app->write_png) {
        app->renderer = SDL_CreateSoftwareRenderer(SDL_CreateSurface(w,h, SDL_PIXELFORMAT_RGBA32));
    } else {
         if (!SDL_Init(SDL_INIT_VIDEO) ||
             !SDL_CreateWindowAndRenderer("iv2d demo", w,h, SDL_WINDOW_RESIZABLE,
                                          &app->window, &app->renderer)) {
            SDL_free(app);
            SDL_Quit();
            return SDL_APP_FAILURE;
        }
        SDL_SetWindowPosition(app->window, 0,0);
    }
    SDL_SetRenderVSync(app->renderer, 1);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *ctx, SDL_AppResult res) {
    struct app *app = ctx;
    (void)res;

    for (int i = 0; i < slides; i++) {
        slide_cleanup(slide[i]);
    }

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
            if (!event->key.repeat) {  // TODO: getting spurious repeats with emcc.
                reset_frametimes(app);
                if (handle_keys(app, (char const[]){(char)event->key.key,0})) {
                    return SDL_APP_SUCCESS;
                }
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
    if (app->animate) {
        app->time = now() - app->start_time;
    }

    SDL_SetRenderDrawBlendMode (app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    int const which = wrap(app->slide, slides);
    struct slide *desc = slide[which];
    float wt = (float)w, ht = (float)h, ft = (float)app->time;
    struct iv2d_region const *region = slide_region(desc, &wt, &ht, &ft);

    struct iv2d_stroke stroke = {.region={iv2d_stroke}, region, 2};
    if (app->stroke) {
        region = &stroke.region;
    }

    double const start = now();
    {
        iv2d_cover(region, 0,0,w,h, app->quality, queue_rect,app);
    }
    app->frametime[app->next_frametime++ % len(app->frametime)] = now() - start;

    double avg_frametime;
    {
        double  sum = 0;
        int nonzero = 0;
        for (int i = 0; i < len(app->frametime); i++) {
            sum     += app->frametime[i];
            nonzero += app->frametime[i] > 0;
        }
        avg_frametime = sum / (double)nonzero;
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
            "%s (%d), %dx%d, quality %d, %d full + %d partial, %.0fµs",
            desc->name, which, w,h, app->quality, app->full, app->quads - app->full,
            app->write_png ? 0 : 1e6 * avg_frametime);

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
