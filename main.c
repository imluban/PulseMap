#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define BG_R 255
#define BG_G 255
#define BG_B 255

#define RED_R 127
#define RED_G 23
#define RED_B 52

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Event event;
    bool running = true;

    SDL_Log("PulseMap: main() started");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Log("PulseMap: SDL initialized");

    if (!SDL_CreateWindowAndRenderer(
            "PulseMap",
            420,
            900,
            SDL_WINDOW_FULLSCREEN,
            &window,
            &renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Log("PulseMap: window and renderer created");

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        int width, height;
        SDL_GetRenderOutputSize(renderer, &width, &height);

        float sx = width / 420.0f;
        float sy = height / 900.0f;

        SDL_SetRenderDrawColor(renderer, BG_R, BG_G, BG_B, 255);
        SDL_RenderClear(renderer);

        SDL_FRect header = {0, 0, (float)width, 220 * sy};
        SDL_SetRenderDrawColor(renderer, RED_R, RED_G, RED_B, 255);
        SDL_RenderFillRect(renderer, &header);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 25 * sx, 55 * sy, "PULSEMAP");
        SDL_RenderDebugText(renderer, 25 * sx, 85 * sy,
                            "EAST DELTA UNIVERSITY");
        SDL_RenderDebugText(renderer, 25 * sx, 135 * sy,
                            "Your campus. Your voice.");

        SDL_FRect card = {20 * sx, 250 * sy, 380 * sx, 180 * sy};
        SDL_SetRenderDrawColor(renderer, 248, 243, 245, 255);
        SDL_RenderFillRect(renderer, &card);

        SDL_SetRenderDrawColor(renderer, RED_R, RED_G, RED_B, 255);
        SDL_RenderDebugText(renderer, 40 * sx, 280 * sy,
                            "CAMPUS PROBLEM RADAR");

        SDL_SetRenderDrawColor(renderer, 65, 65, 65, 255);
        SDL_RenderDebugText(renderer, 40 * sx, 320 * sy,
                            "Campus issues will appear here.");

        SDL_FRect button = {20 * sx, 470 * sy, 380 * sx, 75 * sy};
        SDL_SetRenderDrawColor(renderer, RED_R, RED_G, RED_B, 255);
        SDL_RenderFillRect(renderer, &button);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 115 * sx, 505 * sy,
                            "REPORT A PROBLEM");

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
