#include "slides.h"
#include "len.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static int wrap(int x, int n) {
    return ((x % n) + n) % n;
}

// TODO
//   - tiger

struct quad {
    struct { float x,y; SDL_Color c; } vertex[6];
};

struct app {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    struct quad  *quad;
    int           quads, quad_cap, full;

    int quality, slide;
    int draw_bounds :  1;
    int animate     :  1;
    int stroke      :  1;
    int             : 29;

    double frametime[32];
    int    next_frametime;
    int    :32;

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
    SDL_Color const c = {127,127,127, (uint8_t)(cov*255)};
    app->quad[app->quads++] = (struct quad) {{
        {l,t,c}, {r,t,c}, {l,b,c},
        {r,t,c}, {r,b,c}, {l,b,c},
    }};
}

static void frame(struct app *app) {
    app->quads = app->full = 0;
    if (app->animate) {
        app->time = now() - app->start_time;
    }

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor    (app->renderer, 255,255,255,255);
    SDL_RenderClear           (app->renderer);

    int w,h;
    SDL_GetRendererOutputSize(app->renderer, &w,&h);

    struct slides slides;
    slides_init(&slides, w, h, app->time);

    int const slide = wrap(app->slide, slides.count);
    struct iv2d_region const *region = slides.slide[slide].region;

    struct iv2d_stroke stroke = {.region={iv2d_stroke}, region, 2};
    if (app->stroke) {
        region = &stroke.region;
    }

    double const start = now();
    {
        iv2d_cover(region, 0,0,w,h, app->quality, queue_rect,app);
    }
    app->frametime[app->next_frametime++ % len(app->frametime)] = now() - start;

    slides_fini(&slides);

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
        SDL_SetRenderDrawColor(app->renderer, 255,0,0,31);
        SDL_RenderDrawRectF   (app->renderer, &bounds);
    }

    char title[256];
    snprintf(title, sizeof title,
             "%s (%d), %dx%d, quality %d, %d full + %d partial, %.0f\u00b5s",
             slides.slide[slide].name, slide, w, h, app->quality,
             app->full, app->quads - app->full,
             1e6 * avg_frametime);
    SDL_SetWindowTitle(app->window, title);

    SDL_RenderPresent(app->renderer);
}

int main(int argc, char* argv[]) {
    struct app *app = SDL_calloc(1, sizeof *app);
    app->start_time = now();

    int w=800, h=600;
    for (int i = 1; i < argc; i++) {
        int W,H;
        if (2 == sscanf(argv[i], "%dx%d", &W, &H)) {
            w = W;
            h = H;
            continue;
        }
        (void)handle_keys(app, argv[i]);
    }

    if (0 > SDL_Init(SDL_INIT_VIDEO) ||
        0 > SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE,
                                        &app->window, &app->renderer)) {
        SDL_free(app);
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowPosition(app->window, 0,0);

    for (_Bool done = false; !done;) {
        for (SDL_Event event; SDL_PollEvent(&event);) {
            switch (event.type) {
                default: break;

                case SDL_QUIT:
                    done = 1;
                    break;

                case SDL_KEYDOWN:
                    reset_frametimes(app);
                    if (handle_keys(app, (char const[]){(char)event.key.keysym.sym,0})) {
                        done = 1;
                    }
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        reset_frametimes(app);
                    }
                    break;
            }
        }
        if (!done) {
            frame(app);
        }
    }

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    SDL_free(app->quad);
    SDL_free(app);
    SDL_Quit();
    return 0;
}
