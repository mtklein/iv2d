#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

SDL_AppResult SDL_AppIterate(void *ctx) {
    App *app = ctx;

    int w,h;
    SDL_GetRenderOutputSize(app->renderer, &w,&h);

    SDL_FRect const square = {
        (float)(w - 100) / 2,
        (float)(h - 100) / 2,
        100.0f,
        100.0f,
    };

    SDL_SetRenderDrawColorFloat(app->renderer, 1,1,1,1);
    SDL_RenderClear            (app->renderer         );
    SDL_SetRenderDrawColorFloat(app->renderer, 0,0,0,1);
    SDL_RenderFillRect         (app->renderer, &square);
    SDL_RenderPresent          (app->renderer         );

    return SDL_APP_CONTINUE;
}
