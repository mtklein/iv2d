#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_AppResult SDL_AppInit(void **app, int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    *app = NULL;
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *app, SDL_AppResult res) {
    (void)app;
    (void)res;
}

SDL_AppResult SDL_AppEvent(void *app, SDL_Event *event) {
    (void)app;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *app) {
    (void)app;

    return SDL_APP_CONTINUE;
}
