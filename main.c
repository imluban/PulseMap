
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_W 420
#define SCREEN_H 900

#define RED_R 127
#define RED_G 23
#define RED_B 52

typedef enum {
    SCREEN_DASHBOARD,
    SCREEN_REPORT,
    SCREEN_CONFIRMATION
} AppScreen;

typedef enum {
    FIELD_NONE = -1,
    FIELD_TITLE,
    FIELD_LOCATION,
    FIELD_DESCRIPTION
} ActiveField;

static char title[96] = "";
static char location[96] = "";
static char description[256] = "";

static const char *categories[] = {
    "Facilities", "Cleanliness", "Safety",
    "Internet", "Other"
};

static const char *severities[] = {
    "Low", "Medium", "High", "Critical"
};

static int category = 0;
static int severity = 1;

static char error_message[100] = "";

static void draw_rect(SDL_Renderer *r,
                      float x, float y, float w, float h,
                      Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_FRect rect = {x, y, w, h};

    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    SDL_RenderFillRect(r, &rect);
}

static void draw_text(SDL_Renderer *r,
                      float x, float y,
                      const char *text,
                      Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_SetRenderDrawColor(r, red, green, blue, 255);
    SDL_RenderDebugText(r, x, y, text);
}

static void draw_field(SDL_Renderer *r,
                       float sx, float sy,
                       float y, float h,
                       const char *label,
                       const char *value,
                       bool active)
{
    draw_text(r, 25 * sx, (y - 20) * sy,
              label, 70, 70, 70);

    draw_rect(r, 20 * sx, y * sy,
              380 * sx, h * sy,
              active ? 255 : 248,
              active ? 245 : 243,
              active ? 247 : 245);

    if (value[0] != '\0') {
        draw_text(r, 35 * sx, (y + 22) * sy,
                  value, 45, 45, 45);
    } else {
        draw_text(r, 35 * sx, (y + 22) * sy,
                  "Tap to type...", 145, 145, 145);
    }
}

static void draw_button(SDL_Renderer *r,
                        float sx, float sy,
                        float y, const char *label)
{
    draw_rect(r, 20 * sx, y * sy,
              380 * sx, 75 * sy,
              RED_R, RED_G, RED_B);

    draw_text(r, 110 * sx, (y + 32) * sy,
              label, 255, 255, 255);
}

static void append_text(char *buffer, size_t capacity,
                        const char *input)
{
    size_t current = strlen(buffer);
    size_t incoming = strlen(input);

    if (current + incoming < capacity) {
        memcpy(buffer + current, input, incoming + 1);
    }
}

static void remove_last_character(char *buffer)
{
    size_t length = strlen(buffer);

    if (length == 0) {
        return;
    }

    length--;

    while (length > 0 &&
           ((unsigned char)buffer[length] & 0xC0) == 0x80) {
        length--;
    }

    buffer[length] = '\0';
}

static void activate_field(SDL_Window *window,
                           ActiveField *active,
                           ActiveField next)
{
    *active = next;
    SDL_StartTextInput(window);
}

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Event event;

    bool running = true;

    AppScreen screen = SCREEN_DASHBOARD;
    ActiveField active = FIELD_NONE;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer(
            "PulseMap",
            SCREEN_W,
            SCREEN_H,
            SDL_WINDOW_FULLSCREEN,
            &window,
            &renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    while (running) {
        int output_w, output_h;
        int window_w, window_h;

        SDL_GetRenderOutputSize(renderer, &output_w, &output_h);
        SDL_GetWindowSize(window, &window_w, &window_h);

        float sx = output_w / (float)SCREEN_W;
        float sy = output_h / (float)SCREEN_H;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (event.type == SDL_EVENT_TEXT_INPUT &&
                active != FIELD_NONE) {

                if (active == FIELD_TITLE) {
                    append_text(title, sizeof(title), event.text.text);
                } else if (active == FIELD_LOCATION) {
                    append_text(location, sizeof(location), event.text.text);
                } else if (active == FIELD_DESCRIPTION) {
                    append_text(description, sizeof(description), event.text.text);
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_BACKSPACE &&
                active != FIELD_NONE) {

                if (active == FIELD_TITLE) {
                    remove_last_character(title);
                } else if (active == FIELD_LOCATION) {
                    remove_last_character(location);
                } else if (active == FIELD_DESCRIPTION) {
                    remove_last_character(description);
                }
            }

            float tx = -1.0f;
            float ty = -1.0f;

            if (event.type == SDL_EVENT_FINGER_DOWN) {
                tx = event.tfinger.x * SCREEN_W;
                ty = event.tfinger.y * SCREEN_H;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                event.button.button == SDL_BUTTON_LEFT) {
                tx = event.button.x * SCREEN_W / window_w;
                ty = event.button.y * SCREEN_H / window_h;
            }

            if (tx < 0 || ty < 0) {
                continue;
            }

            if (screen == SCREEN_DASHBOARD) {
                if (tx >= 20 && tx <= 400 &&
                    ty >= 470 && ty <= 545) {

                    screen = SCREEN_REPORT;
                    error_message[0] = '\0';
                }
            }
            else if (screen == SCREEN_REPORT) {

                if (ty >= 40 && ty <= 100 &&
                    tx >= 20 && tx <= 150) {

                    SDL_StopTextInput(window);
                    active = FIELD_NONE;
                    screen = SCREEN_DASHBOARD;
                }
                else if (ty >= 250 && ty <= 325) {
                    activate_field(window, &active, FIELD_TITLE);
                }
                else if (ty >= 350 && ty <= 425) {
                    activate_field(window, &active, FIELD_LOCATION);
                }
                else if (ty >= 450 && ty <= 590) {
                    activate_field(window, &active, FIELD_DESCRIPTION);
                }
                else if (ty >= 620 && ty <= 680) {
                    category = (category + 1) % 5;
                    SDL_StopTextInput(window);
                    active = FIELD_NONE;
                }
                else if (ty >= 700 && ty <= 760) {
                    severity = (severity + 1) % 4;
                    SDL_StopTextInput(window);
                    active = FIELD_NONE;
                }
                else if (ty >= 790 && ty <= 865) {
                    SDL_StopTextInput(window);
                    active = FIELD_NONE;

                    if (title[0] == '\0') {
                        snprintf(error_message, sizeof(error_message),
                                 "Please enter a problem title.");
                    }
                    else if (location[0] == '\0') {
                        snprintf(error_message, sizeof(error_message),
                                 "Please enter a campus location.");
                    }
                    else if (description[0] == '\0') {
                        snprintf(error_message, sizeof(error_message),
                                 "Please describe the problem.");
                    }
                    else {
                        error_message[0] = '\0';
                        screen = SCREEN_CONFIRMATION;
                    }
                }
            }
            else if (screen == SCREEN_CONFIRMATION) {
                if (ty >= 470 && ty <= 550) {
                    title[0] = '\0';
                    location[0] = '\0';
                    description[0] = '\0';

                    category = 0;
                    severity = 1;

                    screen = SCREEN_DASHBOARD;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (screen == SCREEN_DASHBOARD) {
            draw_rect(renderer, 0, 0, output_w, 220 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 25 * sx, 55 * sy,
                      "PULSEMAP", 255, 255, 255);

            draw_text(renderer, 25 * sx, 85 * sy,
                      "EAST DELTA UNIVERSITY", 255, 255, 255);

            draw_text(renderer, 25 * sx, 135 * sy,
                      "Your campus. Your voice.", 255, 255, 255);

            draw_rect(renderer, 20 * sx, 250 * sy,
                      380 * sx, 180 * sy, 248, 243, 245);

            draw_text(renderer, 40 * sx, 280 * sy,
                      "CAMPUS PROBLEM RADAR",
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 40 * sx, 320 * sy,
                      "Campus issues will appear here.",
                      65, 65, 65);

            draw_button(renderer, sx, sy, 470,
                        "REPORT A PROBLEM");
        }
        else if (screen == SCREEN_REPORT) {
            draw_rect(renderer, 0, 0, output_w, 150 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 25 * sx, 55 * sy,
                      "< BACK", 255, 255, 255);

            draw_text(renderer, 25 * sx, 105 * sy,
                      "REPORT A PROBLEM", 255, 255, 255);

            draw_field(renderer, sx, sy, 250, 55,
                       "PROBLEM TITLE", title,
                       active == FIELD_TITLE);

            draw_field(renderer, sx, sy, 350, 55,
                       "CAMPUS LOCATION", location,
                       active == FIELD_LOCATION);

            draw_field(renderer, sx, sy, 450, 110,
                       "DESCRIPTION", description,
                       active == FIELD_DESCRIPTION);

            draw_text(renderer, 25 * sx, 610 * sy,
                      "CATEGORY (TAP TO CHANGE)",
                      70, 70, 70);

            draw_rect(renderer, 20 * sx, 620 * sy,
                      380 * sx, 55 * sy, 248, 243, 245);

            draw_text(renderer, 35 * sx, 642 * sy,
                      categories[category], RED_R, RED_G, RED_B);

            draw_text(renderer, 25 * sx, 690 * sy,
                      "SEVERITY (TAP TO CHANGE)",
                      70, 70, 70);

            draw_rect(renderer, 20 * sx, 700 * sy,
                      380 * sx, 55 * sy, 248, 243, 245);

            draw_text(renderer, 35 * sx, 722 * sy,
                      severities[severity], RED_R, RED_G, RED_B);

            if (error_message[0] != '\0') {
                draw_text(renderer, 25 * sx, 775 * sy,
                          error_message, 200, 35, 35);
            }

            draw_button(renderer, sx, sy, 790,
                        "SUBMIT REPORT");
        }
        else if (screen == SCREEN_CONFIRMATION) {
            draw_rect(renderer, 0, 0, output_w, 220 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 25 * sx, 65 * sy,
                      "PULSEMAP", 255, 255, 255);

            draw_text(renderer, 25 * sx, 110 * sy,
                      "REPORT VALIDATED", 255, 255, 255);

            draw_text(renderer, 25 * sx, 300 * sy,
                      "Your report passed validation.",
                      65, 65, 65);

            draw_text(renderer, 25 * sx, 345 * sy,
                      "Backend submission comes next.",
                      65, 65, 65);

            draw_button(renderer, sx, sy, 470,
                        "RETURN TO DASHBOARD");
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
