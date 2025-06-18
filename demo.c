#include "cleanup.h"
#include "iv2d.h"
#include "iv2d_regions.h"
#include "iv2d_vm.h"
#include "len.h"
#include "prospero.h"
#include "stb/stb_image_write.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdint.h>
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
    struct { float x,y; SDL_Color c; } vertex[6];
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
    SDL_Color const c = {127,127,127, (uint8_t)(cov*255)};
    app->quad[app->quads++] = (struct quad) {{
        {l,t,c}, {r,t,c}, {l,b,c},
        {r,t,c}, {r,b,c}, {l,b,c},
    }};
}

static struct iv2d_halfplane halfplane_from(float x0, float y0, float x1, float y1) {
    float const dx = x1 - x0,
                dy = y1 - y0,
              norm = 1 / sqrtf(dx*dx + dy*dy),
                nx = +dy*norm,
                ny = -dx*norm,
                 d = nx*x0 + ny*y0;
    return (struct iv2d_halfplane){.region={iv2d_halfplane}, nx,ny,d};
}

static struct iv2d_setop intersect_halfplanes(struct iv2d_halfplane const hp[], int n,
                                              struct iv2d_region const *region[]) {
    for (int i = 0; i < n; i++) {
        region[i] = &hp[i].region;
    }
    return (struct iv2d_setop){.region={iv2d_intersect}, region, n};
}

static _Bool frame(struct app *app) {
    app->quads = app->full = 0;
    if (app->animate) {
        app->time = now() - app->start_time;
    }

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor    (app->renderer, 255,255,255,255);
    SDL_RenderClear           (app->renderer);

    int w,h;
    SDL_GetRendererOutputSize(app->renderer, &w,&h);

    float const cx = 0.5f * (float)w,
                cy = 0.5f * (float)h,
                cr = 0.5f*fminf(cx,cy),
                th = (float)app->time,
                ox = cx + (300-cx)*cosf(th) - (200-cy)*sinf(th),
                oy = cy + (200-cy)*cosf(th) + (300-cx)*sinf(th);

    struct iv2d_circle const center = {.region={iv2d_circle}, cx, cy, cr},
                             orbit  = {.region={iv2d_circle}, ox, oy, 100};
    struct iv2d_invert const invorb = {.region={iv2d_invert}, &orbit.region};

    struct iv2d_region const *center_orbit [] = {&center.region, & orbit.region},
                             *center_invorb[] = {&center.region, &invorb.region};
    struct iv2d_setop const
        union_     = {.region={iv2d_union    }, center_orbit , len(center_orbit )},
        intersect  = {.region={iv2d_intersect}, center_orbit , len(center_orbit )},
        difference = {.region={iv2d_intersect}, center_invorb, len(center_invorb)};

    struct iv2d_capsule capsule = {.region={iv2d_capsule}, ox,oy, cx,cy, 4};

    struct iv2d_halfplane halfplane = halfplane_from(ox,oy, cx,cy);

    struct iv2d_halfplane hp[7];
    for (int i = 0; i < len(hp); i++) {
        double const pi = atan(1)*4;
        hp[i] = halfplane_from(cx + 100 * (float)cos(app->time + (i  ) * 2*pi/len(hp)),
                               cy + 100 * (float)sin(app->time + (i  ) * 2*pi/len(hp)),
                               cx + 100 * (float)cos(app->time + (i+1) * 2*pi/len(hp)),
                               cy + 100 * (float)sin(app->time + (i+1) * 2*pi/len(hp)));
    }
    struct iv2d_region const *ngon_region[len(hp)];
    struct iv2d_setop ngon = intersect_halfplanes(hp, len(hp), ngon_region);

    char ngon_name[128];
    snprintf(ngon_name, sizeof ngon_name, "%d-gon", len(hp));

    __attribute__((cleanup(free_cleanup)))
    struct iv2d_region const *vm_union;
    {
        struct iv2d_builder *b = iv2d_builder();

        int center_circle;
        {
            int const dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&cx))),
                      dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&cy))),
                      len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
            center_circle = iv2d_sub(b, len, iv2d_uni(b,&cr));
        }
        int orbit_circle;
        {
            int const dx2 = iv2d_square(b, iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&ox))),
                      dy2 = iv2d_square(b, iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&oy))),
                      len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
            orbit_circle  = iv2d_sub(b, len, iv2d_imm(b,100));
        }
        vm_union = iv2d_ret(b, iv2d_min(b, center_circle,orbit_circle));
    }

    static struct iv2d_region const *prospero = NULL;
    struct {
        struct iv2d_region const *region;
        char const               *name;
    } slides[] = {
        {&union_    .region, "union"     },
        {&intersect .region, "intersect" },
        {&difference.region, "difference"},
        {&capsule   .region, "capsule"   },
        {&halfplane .region, "halfplane" },
        {&ngon      .region,  ngon_name  },
        {vm_union,           "vm union"  },
        {prospero,           "prospero"  },
    };
    int const slide = wrap(app->slide, len(slides));
    struct iv2d_region const *region = slides[slide].region;

    float W = (float)w,
          H = (float)h;
    if (prospero == NULL && 0 == strcmp(slides[slide].name, "prospero")) {
        region = prospero = prospero_region(&W,&H);
    }

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
        SDL_SetRenderDrawColor(app->renderer, 255,0,0,31);
        SDL_RenderDrawRectF   (app->renderer, &bounds);
    }

    if (app->window) {
        char title[256];
        snprintf(title, sizeof title,
                 "%s (%d), %dx%d, quality %d, %d full + %d partial, %.0f\u00b5s",
                 slides[slide].name, slide, w, h, app->quality,
                 app->full, app->quads - app->full,
                 app->write_png ? 0 : 1e6 * avg_frametime);
        SDL_SetWindowTitle(app->window, title);
    }

    if (app->write_png) {
        SDL_Surface *rgba = SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
        SDL_RenderReadPixels(app->renderer, NULL,
                             SDL_PIXELFORMAT_RGBA32, rgba->pixels, rgba->pitch);
        stbi_write_png_to_func(write_to_stdout, NULL, w,h,4, rgba->pixels, rgba->pitch);
        SDL_FreeSurface(rgba);
        return 1;
    }

    SDL_RenderPresent(app->renderer);
    return 0;
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

    if (app->write_png) {
        app->renderer = SDL_CreateSoftwareRenderer(
                SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32));
    } else {
         if (0 > SDL_Init(SDL_INIT_VIDEO) ||
             0 > SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE,
                                             &app->window, &app->renderer)) {
            SDL_free(app);
            SDL_Quit();
            return 1;
        }
        SDL_SetWindowPosition(app->window, 0,0);
    }

    for (_Bool done = 0; !done;) {
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
            done = frame(app);
        }
    }

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow  (app->window);
    SDL_free(app->quad);
    SDL_free(app);
    SDL_Quit();
    return 0;
}
