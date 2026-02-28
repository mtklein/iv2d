#include "slides.h"
#include "cleanup.h"
#include "len.h"
#include "prospero.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang attribute push (__attribute__((no_sanitize("integer", "undefined"))), apply_to=function)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#pragma clang attribute pop
#pragma clang diagnostic pop
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct iv2d_halfplane halfplane_from(float x0, float y0, float x1, float y1) {
    float const dx = x1 - x0,
                dy = y1 - y0,
              norm = 1 / sqrtf(dx*dx + dy*dy),
                nx = +dy*norm,
                ny = -dx*norm,
                 d = nx*x0 + ny*y0;
    return (struct iv2d_halfplane){.region={iv2d_halfplane}, nx,ny,d};
}

static int capsule_sdf(struct iv2d_builder *b,
                       float x0,float y0, float x1,float y1, float r) {
    float const dx = x1 - x0,
                dy = y1 - y0,
                inv_len2 = 1 / (dx*dx + dy*dy);

    int px = iv2d_sub(b, iv2d_x(b), iv2d_imm(b, x0)),
        py = iv2d_sub(b, iv2d_y(b), iv2d_imm(b, y0));

    int dot = iv2d_add(b, iv2d_mul(b, px, iv2d_imm(b, dx)),
                           iv2d_mul(b, py, iv2d_imm(b, dy)));
    int t = iv2d_mul(b, dot, iv2d_imm(b, inv_len2));

    int h = iv2d_max(b, iv2d_imm(b,0), iv2d_min(b, t, iv2d_imm(b,1)));

    int hx = iv2d_sub(b, px, iv2d_mul(b, h, iv2d_imm(b, dx))),
        hy = iv2d_sub(b, py, iv2d_mul(b, h, iv2d_imm(b, dy)));

    int sdf_len = iv2d_sqrt(b, iv2d_add(b, iv2d_mul(b,hx,hx), iv2d_mul(b,hy,hy)));
    return iv2d_sub(b, sdf_len, iv2d_imm(b, r));
}

static char* load_helvetica(stbtt_fontinfo *font) {
    char const *path[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Helvetica.ttf",
        "/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Helvetica.ttf",
    };

    FILE *f = NULL;
    for (int i = 0; i < len(path) && !f; i++) {
        f = fopen(path[i], "rb");
    }
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc((size_t)size);
    fread(data, 1, (size_t)size, f);
    fclose(f);

    void const *font_data = data;
    stbtt_InitFont(font, font_data, stbtt_GetFontOffsetForIndex(font_data,0));
    return data;
}

static struct iv2d_region const* hamburgefonsiv_region(void) {
    static struct iv2d_region const *region;
    if (region) {
        return region;
    }

    stbtt_fontinfo font;
    char *data = load_helvetica(&font);
    if (!data) {
        return NULL;
    }

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
    float const scale = stbtt_ScaleForPixelHeight(&font, 100);
    float const baseline = (float)ascent * scale;

    struct iv2d_builder *b = iv2d_builder();
    int out = 0;
    _Bool first = 1;

    int prev = 0;
    float pen_x = 0;
    char const text[] = "Hamburgefonsiv";
    for (char const *c = text; *c; c++) {
        int kern = stbtt_GetCodepointKernAdvance(&font, prev, *c);
        pen_x += (float)kern * scale;
        prev = *c;

        int glyph = stbtt_FindGlyphIndex(&font, *c);
        stbtt_vertex *verts;
        int num_verts = stbtt_GetGlyphShape(&font, glyph, &verts);
        int *counts, n;
        stbtt__point *pts = stbtt_FlattenCurves(verts, num_verts, 0.35f,
                                               &counts, &n, NULL);
        int idx = 0;
        for (int ci = 0; ci < n; ci++) {
            float x0 = pen_x + pts[idx].x * scale;
            float y0 = baseline - pts[idx].y * scale;
            for (int i = 1; i < counts[ci]; i++) {
                float x1 = pen_x + pts[idx+i].x * scale;
                float y1 = baseline - pts[idx+i].y * scale;
                int seg = capsule_sdf(b, x0,y0, x1,y1, 2.0f);
                if (first) {
                    out = seg;
                    first = 0;
                } else {
                    out = iv2d_min(b, out, seg);
                }
                x0 = x1;
                y0 = y1;
            }
            idx += counts[ci];
        }
        STBTT_free(pts, NULL);
        STBTT_free(counts, NULL);
        stbtt_FreeShape(&font, verts);

        int adv;
        stbtt_GetGlyphHMetrics(&font, glyph, &adv, NULL);
        pen_x += (float)adv * scale;
    }

    free(data);

    region = iv2d_ret(b, out);
    return region;
}

static struct iv2d_region const* hamburgefonsiv_sdf_region(void) {
    static struct iv2d_region const *region;
    if (region) {
        return region;
    }

    stbtt_fontinfo font;
    char *data = load_helvetica(&font);
    if (!data) {
        return NULL;
    }

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

    float const scale = stbtt_ScaleForPixelHeight(&font, 100),
                baseline = (float)ascent * scale,
                onedge_value = 180,
                pixel_dist_scale = 36,
                outside = onedge_value / pixel_dist_scale;
    int const padding = 8;
    char const text[] = "Hamburgefonsiv";

    int prev = 0;
    float pen_x = 0,
          l = +1.0f/0.0f,
          t = +1.0f/0.0f,
          r = -1.0f/0.0f,
          b = -1.0f/0.0f;
    for (char const *c = text; *c; c++) {
        int const codepoint = *c,
                  glyph = stbtt_FindGlyphIndex(&font, codepoint);
        int const kern = stbtt_GetCodepointKernAdvance(&font, prev, codepoint);
        pen_x += kern * scale;
        prev = codepoint;

        int w,h,xoff,yoff;
        unsigned char *sdf = stbtt_GetGlyphSDF(&font, scale, glyph, padding,
                                               (unsigned char)onedge_value, pixel_dist_scale,
                                               &w,&h, &xoff,&yoff);
        if (sdf) {
            l = fminf(l, pen_x + xoff);
            t = fminf(t, baseline + yoff);
            r = fmaxf(r, pen_x + xoff + w);
            b = fmaxf(b, baseline + yoff + h);
            stbtt_FreeSDF(sdf, NULL);
        }

        int adv;
        stbtt_GetGlyphHMetrics(&font, glyph, &adv, NULL);
        pen_x += (float)adv * scale;
    }

    float const text_w = ceilf(r - l),
                text_h = ceilf(b - t);
    int const width = (int)text_w,
              height = (int)text_h;
    _Float16 *buf = malloc((size_t)(width * height) * sizeof *buf);
    for (int i = 0; i < width * height; i++) {
        buf[i] = (_Float16)outside;
    }

    prev = 0;
    pen_x = 0;
    for (char const *c = text; *c; c++) {
        int const codepoint = *c,
                  glyph = stbtt_FindGlyphIndex(&font, codepoint);
        int const kern = stbtt_GetCodepointKernAdvance(&font, prev, codepoint);
        pen_x += kern * scale;
        prev = codepoint;

        int w,h,xoff,yoff;
        unsigned char *sdf = stbtt_GetGlyphSDF(&font, scale, glyph, padding,
                                               (unsigned char)onedge_value, pixel_dist_scale,
                                               &w,&h, &xoff,&yoff);
        if (sdf) {
            float const glyph_x = floorf(pen_x + xoff - l),
                        glyph_y = floorf(baseline + yoff - t);
            int const gx = (int)glyph_x,
                      gy = (int)glyph_y;
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    float const d = (onedge_value - sdf[y*w + x]) / pixel_dist_scale;
                    _Float16 *dst = buf + (gy+y) * width + (gx+x);
                    float const current = (float)*dst;
                    *dst = (_Float16)fminf(current, d);
                }
            }
            stbtt_FreeSDF(sdf, NULL);
        }

        int adv;
        stbtt_GetGlyphHMetrics(&font, glyph, &adv, NULL);
        pen_x += (float)adv * scale;
    }

    free(data);

    struct iv2d_sdf *text_sdf = malloc(sizeof *text_sdf);
    *text_sdf = (struct iv2d_sdf){
        .region={iv2d_sdf},
        .sdf=buf,
        .x=l,
        .y=t,
        .w=width,
        .h=height,
    };
    region = &text_sdf->region;
    return region;
}

void slides_init(struct slides *s, int w, int h, double time) {
    s->cx = 0.5f * (float)w;
    s->cy = 0.5f * (float)h;
    s->cr = 0.5f*fminf(s->cx, s->cy);
    float const th = (float)time;
    s->ox = s->cx + (300-s->cx)*cosf(th) - (200-s->cy)*sinf(th);
    s->oy = s->cy + (200-s->cy)*cosf(th) + (300-s->cx)*sinf(th);

    s->center = (struct iv2d_circle){.region={iv2d_circle}, s->cx, s->cy, s->cr};
    s->orbit  = (struct iv2d_circle){.region={iv2d_circle}, s->ox, s->oy, 100};
    s->invorb = (struct iv2d_invert){.region={iv2d_invert}, &s->orbit.region};

    s->center_orbit [0] = &s->center.region;
    s->center_orbit [1] = &s->orbit.region;
    s->center_invorb[0] = &s->center.region;
    s->center_invorb[1] = &s->invorb.region;

    s->union_     = (struct iv2d_setop){.region={iv2d_union    }, s->center_orbit , 2};
    s->intersect  = (struct iv2d_setop){.region={iv2d_intersect}, s->center_orbit , 2};
    s->difference = (struct iv2d_setop){.region={iv2d_intersect}, s->center_invorb, 2};

    s->capsule   = (struct iv2d_capsule){.region={iv2d_capsule}, s->ox,s->oy, s->cx,s->cy, 4};
    s->halfplane = halfplane_from(s->ox, s->oy, s->cx, s->cy);

    for (int i = 0; i < SLIDES_HP; i++) {
        double const pi = atan(1)*4;
        s->hp[i] = halfplane_from(
            s->cx + 100 * (float)cos(time + (i  ) * 2*pi/SLIDES_HP),
            s->cy + 100 * (float)sin(time + (i  ) * 2*pi/SLIDES_HP),
            s->cx + 100 * (float)cos(time + (i+1) * 2*pi/SLIDES_HP),
            s->cy + 100 * (float)sin(time + (i+1) * 2*pi/SLIDES_HP));
    }
    for (int i = 0; i < SLIDES_HP; i++) {
        s->ngon_region[i] = &s->hp[i].region;
    }
    s->ngon = (struct iv2d_setop){.region={iv2d_intersect}, s->ngon_region, SLIDES_HP};

    snprintf(s->ngon_name, sizeof s->ngon_name, "%d-gon", SLIDES_HP);

    {
        struct iv2d_builder *b = iv2d_builder();

        int center_circle;
        {
            int const dx = iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&s->cx)),
                      dy = iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&s->cy)),
                     dx2 = iv2d_mul(b, dx,dx),
                     dy2 = iv2d_mul(b, dy,dy),
                     len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
            center_circle = iv2d_sub(b, len, iv2d_uni(b,&s->cr));
        }
        int orbit_circle;
        {
            int const dx = iv2d_sub(b, iv2d_x(b), iv2d_uni(b,&s->ox)),
                      dy = iv2d_sub(b, iv2d_y(b), iv2d_uni(b,&s->oy)),
                     dx2 = iv2d_mul(b, dx,dx),
                     dy2 = iv2d_mul(b, dy,dy),
                     len = iv2d_sqrt(b, iv2d_add(b, dx2, dy2));
            orbit_circle  = iv2d_sub(b, len, iv2d_imm(b,100));
        }
        s->vm_union = iv2d_ret(b, iv2d_min(b, center_circle,orbit_circle));
    }

    static float prospero_W, prospero_H;
    static struct iv2d_region const *prospero_r;
    static struct iv2d_region const *helvetica;
    static struct iv2d_region const *helvetica_sdf;
    prospero_W = (float)w;
    prospero_H = (float)h;
    if (!prospero_r)    prospero_r    = prospero_region(&prospero_W, &prospero_H);
    if (!helvetica)     helvetica     = hamburgefonsiv_region();
    if (!helvetica_sdf) helvetica_sdf = hamburgefonsiv_sdf_region();

    s->slide[0] = (struct slide){&s->union_    .region, "union"     };
    s->slide[1] = (struct slide){&s->intersect .region, "intersect" };
    s->slide[2] = (struct slide){&s->difference.region, "difference"};
    s->slide[3] = (struct slide){&s->capsule   .region, "capsule"   };
    s->slide[4] = (struct slide){&s->halfplane .region, "halfplane" };
    s->slide[5] = (struct slide){&s->ngon      .region,  s->ngon_name};
    s->slide[6] = (struct slide){s->vm_union,            "vm union"  };
    s->slide[7] = (struct slide){helvetica,              "helvetica" };
    s->slide[8] = (struct slide){helvetica_sdf,          "helvetica sdf"};
    s->slide[9] = (struct slide){prospero_r,             "prospero"  };
    s->count = 10;
}

void slides_fini(struct slides *s) {
    free_cleanup(&s->vm_union);
    s->vm_union = NULL;
}
