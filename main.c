
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define SCREEN_W 420
#define SCREEN_H 900

#define RED_R 127
#define RED_G 23
#define RED_B 52

#define WHITE_R 255
#define WHITE_G 255
#define WHITE_B 255

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
    "Facilities",
    "Cleanliness",
    "Safety",
    "Internet",
    "Other"
};

static const char *severities[] = {
    "Low",
    "Medium",
    "High",
    "Critical"
};

static int category = 0;
static int severity = 1;

static char error_message[100] = "";

/* ---------------------------------------------------------
   DRAWING HELPERS
   --------------------------------------------------------- */

static void draw_rect(SDL_Renderer *renderer,
                      float x, float y,
                      float w, float h,
                      Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_FRect rect = {x, y, w, h};

    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderFillRect(renderer, &rect);
}

static void draw_text(SDL_Renderer *renderer,
                      float x, float y,
                      const char *text,
                      Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
    SDL_RenderDebugText(renderer, x, y, text);
}

static void draw_field(SDL_Renderer *renderer,
                       float sx, float sy,
                       float y, float h,
                       const char *label,
                       const char *value,
                       bool active)
{
    draw_text(renderer,
              25 * sx,
              (y - 20) * sy,
              label,
              70, 70, 70);

    draw_rect(renderer,
              20 * sx,
              y * sy,
              380 * sx,
              h * sy,
              active ? 255 : 248,
              active ? 245 : 243,
              active ? 247 : 245);

    if (value[0] != '\0') {
        draw_text(renderer,
                  35 * sx,
                  (y + 22) * sy,
                  value,
                  45, 45, 45);
    } else {
        draw_text(renderer,
                  35 * sx,
                  (y + 22) * sy,
                  "Tap to type...",
                  145, 145, 145);
    }
}

static void draw_button(SDL_Renderer *renderer,
                        float sx, float sy,
                        float y,
                        const char *label)
{
    draw_rect(renderer,
              20 * sx,
              y * sy,
              380 * sx,
              75 * sy,
              RED_R, RED_G, RED_B);

    draw_text(renderer,
              110 * sx,
              (y + 32) * sy,
              label,
              255, 255, 255);
}

static void draw_done_button(SDL_Renderer *renderer,
                             float sx, float sy)
{
    draw_rect(renderer,
              325 * sx,
              35 * sy,
              75 * sx,
              45 * sy,
              255, 255, 255);

    draw_text(renderer,
              342 * sx,
              52 * sy,
              "DONE",
              RED_R, RED_G, RED_B);
}

/* ---------------------------------------------------------
   INPUT HELPERS
   --------------------------------------------------------- */

static char *get_active_buffer(ActiveField field)
{
    switch (field) {
        case FIELD_TITLE:
            return title;

        case FIELD_LOCATION:
            return location;

        case FIELD_DESCRIPTION:
            return description;

        default:
            return NULL;
    }
}

static size_t get_active_capacity(ActiveField field)
{
    switch (field) {
        case FIELD_TITLE:
            return sizeof(title);

        case FIELD_LOCATION:
            return sizeof(location);

        case FIELD_DESCRIPTION:
            return sizeof(description);

        default:
            return 0;
    }
}

/*
 * Handle both normal text events and cumulative text
 * events received from some Android keyboards.
 */
static void process_text_input(ActiveField field,
                               const char *input)
{
    char *buffer = get_active_buffer(field);
    size_t capacity = get_active_capacity(field);

    if (buffer == NULL || input == NULL || capacity == 0) {
        return;
    }

    if (input[0] == '\0') {
        return;
    }

    size_t current_length = strlen(buffer);
    size_t incoming_length = strlen(input);

    /*
     * Cumulative event:
     * Existing: Library
     * Incoming: LibraryL
     *
     * Replace the buffer instead of appending.
     */
    if (current_length > 0 &&
        incoming_length >= current_length &&
        strncmp(input, buffer, current_length) == 0) {

        if (incoming_length < capacity) {
            memcpy(buffer, input, incoming_length + 1);
        }

        return;
    }

    /*
     * Ignore identical events.
     */
    if (strcmp(buffer, input) == 0) {
        return;
    }

    /*
     * Normal event: append newly committed text.
     */
    if (current_length + incoming_length < capacity) {
        memcpy(buffer + current_length,
               input,
               incoming_length + 1);
    }
}

static void remove_last_character(char *buffer)
{
    size_t length = strlen(buffer);

    if (length == 0) {
        return;
    }

    length--;

    /* Preserve UTF-8 characters. */
    while (length > 0 &&
           ((unsigned char)buffer[length] & 0xC0) == 0x80) {
        length--;
    }

    buffer[length] = '\0';
}

/*
 * Completely stop the current text-input session.
 */
static void stop_text_input(SDL_Window *window,
                            ActiveField *active)
{
    if (*active != FIELD_NONE) {
        SDL_StopTextInput(window);
        SDL_ClearComposition(window);
    }

    *active = FIELD_NONE;
}

/*
 * Start a fresh Android keyboard session when switching fields.
 */
static void activate_field(SDL_Window *window,
                           ActiveField *active,
                           ActiveField next)
{
    if (*active == next) {
        return;
    }

    /*
     * End the old field's IME session first.
     */
    if (*active != FIELD_NONE) {
        SDL_StopTextInput(window);
        SDL_ClearComposition(window);
    }

    *active = next;

    /*
     * Start a fresh session for the new field.
     */
    SDL_StartTextInput(window);
}

/* ---------------------------------------------------------
   REPORT HELPERS
   --------------------------------------------------------- */

static void clear_report(void)
{
    title[0] = '\0';
    location[0] = '\0';
    description[0] = '\0';

    category = 0;
    severity = 1;

    error_message[0] = '\0';
}

static void submit_report(AppScreen *screen)
{
    if (title[0] == '\0') {
        snprintf(error_message,
                 sizeof(error_message),
                 "Please enter a problem title.");
    }
    else if (location[0] == '\0') {
        snprintf(error_message,
                 sizeof(error_message),
                 "Please enter a campus location.");
    }
    else if (description[0] == '\0') {
        snprintf(error_message,
                 sizeof(error_message),
                 "Please describe the problem.");
    }
    else {
        error_message[0] = '\0';
        *screen = SCREEN_CONFIRMATION;
    }
}

/* ---------------------------------------------------------
   MAIN APPLICATION
   --------------------------------------------------------- */

int main(int argc, char *argv[])
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Event event;

    bool running = true;
    bool keyboard_visible = false;

    AppScreen screen = SCREEN_DASHBOARD;
    ActiveField active = FIELD_NONE;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL initialization failed: %s",
                SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer(
            "PulseMap",
            SCREEN_W,
            SCREEN_H,
            SDL_WINDOW_FULLSCREEN,
            &window,
            &renderer)) {

        SDL_Log("Window creation failed: %s",
                SDL_GetError());

        SDL_Quit();
        return 1;
    }

    while (running) {

        int output_w = SCREEN_W;
        int output_h = SCREEN_H;

        int window_w = SCREEN_W;
        int window_h = SCREEN_H;

        SDL_GetRenderOutputSize(renderer,
                                &output_w,
                                &output_h);

        SDL_GetWindowSize(window,
                          &window_w,
                          &window_h);

        if (window_w <= 0) window_w = SCREEN_W;
        if (window_h <= 0) window_h = SCREEN_H;

        float sx = output_w / (float)SCREEN_W;
        float sy = output_h / (float)SCREEN_H;

        keyboard_visible = SDL_ScreenKeyboardShown(window);

        /* -------------------------------------------------
           EVENT HANDLING
           ------------------------------------------------- */

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }

            /*
             * Text committed by the Android keyboard.
             */
            if (event.type == SDL_EVENT_TEXT_INPUT &&
                active != FIELD_NONE) {

                process_text_input(active, event.text.text);
                continue;
            }

            /*
             * Backspace.
             */
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_BACKSPACE &&
                active != FIELD_NONE) {

                char *buffer = get_active_buffer(active);

                if (buffer != NULL) {
                    remove_last_character(buffer);
                }

                continue;
            }

            /*
             * Convert touch or mouse coordinates.
             */
            float tx = -1.0f;
            float ty = -1.0f;

            if (event.type == SDL_EVENT_FINGER_DOWN) {

                tx = event.tfinger.x * SCREEN_W;
                ty = event.tfinger.y * SCREEN_H;

            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                     event.button.button == SDL_BUTTON_LEFT) {

                tx = event.button.x * SCREEN_W / window_w;
                ty = event.button.y * SCREEN_H / window_h;
            }

            if (tx < 0 || ty < 0) {
                continue;
            }

            /* ---------------------------------------------
               DASHBOARD
               --------------------------------------------- */

            if (screen == SCREEN_DASHBOARD) {

                if (tx >= 20 && tx <= 400 &&
                    ty >= 470 && ty <= 545) {

                    screen = SCREEN_REPORT;
                    error_message[0] = '\0';
                }
            }

            /* ---------------------------------------------
               REPORT FORM
               --------------------------------------------- */

            else if (screen == SCREEN_REPORT) {

                /*
                 * Back button.
                 */
                if (ty >= 40 && ty <= 100 &&
                    tx >= 20 && tx <= 150) {

                    stop_text_input(window, &active);
                    screen = SCREEN_DASHBOARD;
                    continue;
                }

                /*
                 * Done button.
                 */
                if (keyboard_visible &&
                    ty >= 30 && ty <= 90 &&
                    tx >= 315 && tx <= 405) {

                    stop_text_input(window, &active);
                    continue;
                }

                /*
                 * Compact layout while keyboard is visible.
                 */
                if (keyboard_visible) {

                    if (ty >= 155 && ty <= 245) {

                        activate_field(window,
                                       &active,
                                       FIELD_TITLE);

                    }
                    else if (ty >= 250 && ty <= 345) {

                        activate_field(window,
                                       &active,
                                       FIELD_LOCATION);

                    }
                    else if (ty >= 350 && ty <= 485) {

                        activate_field(window,
                                       &active,
                                       FIELD_DESCRIPTION);
                    }
                }

                /*
                 * Normal layout while keyboard is hidden.
                 */
                else {

                    if (ty >= 250 && ty <= 325) {

                        activate_field(window,
                                       &active,
                                       FIELD_TITLE);

                    }
                    else if (ty >= 350 && ty <= 425) {

                        activate_field(window,
                                       &active,
                                       FIELD_LOCATION);

                    }
                    else if (ty >= 450 && ty <= 590) {

                        activate_field(window,
                                       &active,
                                       FIELD_DESCRIPTION);

                    }
                    else if (ty >= 620 && ty <= 680) {

                        stop_text_input(window, &active);
                        category = (category + 1) % 5;

                    }
                    else if (ty >= 700 && ty <= 760) {

                        stop_text_input(window, &active);
                        severity = (severity + 1) % 4;

                    }
                    else if (ty >= 790 && ty <= 865) {

                        stop_text_input(window, &active);
                        submit_report(&screen);
                    }
                }
            }

            /* ---------------------------------------------
               CONFIRMATION
               --------------------------------------------- */

            else if (screen == SCREEN_CONFIRMATION) {

                if (ty >= 470 && ty <= 550) {

                    clear_report();
                    screen = SCREEN_DASHBOARD;
                }
            }
        }

        keyboard_visible = SDL_ScreenKeyboardShown(window);

        /* -------------------------------------------------
           RENDERING
           ------------------------------------------------- */

        SDL_SetRenderDrawColor(renderer,
                               WHITE_R,
                               WHITE_G,
                               WHITE_B,
                               255);

        SDL_RenderClear(renderer);

        /* ---------------------------------------------
           DASHBOARD SCREEN
           --------------------------------------------- */

        if (screen == SCREEN_DASHBOARD) {

            draw_rect(renderer,
                      0, 0,
                      output_w,
                      220 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer,
                      25 * sx,
                      55 * sy,
                      "PULSEMAP",
                      255, 255, 255);

            draw_text(renderer,
                      25 * sx,
                      85 * sy,
                      "EAST DELTA UNIVERSITY",
                      255, 255, 255);

            draw_text(renderer,
                      25 * sx,
                      135 * sy,
                      "Your campus. Your voice.",
                      255, 255, 255);

            draw_rect(renderer,
                      20 * sx,
                      250 * sy,
                      380 * sx,
                      180 * sy,
                      248, 243, 245);

            draw_text(renderer,
                      40 * sx,
                      280 * sy,
                      "CAMPUS PROBLEM RADAR",
                      RED_R, RED_G, RED_B);

            draw_text(renderer,
                      40 * sx,
                      320 * sy,
                      "Campus issues will appear here.",
                      65, 65, 65);

            draw_button(renderer,
                        sx, sy,
                        470,
                        "REPORT A PROBLEM");
        }

        /* ---------------------------------------------
           REPORT SCREEN
           --------------------------------------------- */

        else if (screen == SCREEN_REPORT) {

            draw_rect(renderer,
                      0, 0,
                      output_w,
                      150 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer,
                      25 * sx,
                      55 * sy,
                      "< BACK",
                      255, 255, 255);

            draw_text(renderer,
                      25 * sx,
                      105 * sy,
                      "REPORT A PROBLEM",
                      255, 255, 255);

            /*
             * Compact layout while keyboard is visible.
             */
            if (keyboard_visible) {

                draw_done_button(renderer, sx, sy);

                draw_field(renderer,
                           sx, sy,
                           180, 55,
                           "PROBLEM TITLE",
                           title,
                           active == FIELD_TITLE);

                draw_field(renderer,
                           sx, sy,
                           275, 55,
                           "CAMPUS LOCATION",
                           location,
                           active == FIELD_LOCATION);

                draw_field(renderer,
                           sx, sy,
                           375, 100,
                           "DESCRIPTION",
                           description,
                           active == FIELD_DESCRIPTION);

                draw_text(renderer,
                          25 * sx,
                          500 * sy,
                          "Finish typing, then tap DONE.",
                          100, 100, 100);
            }

            /*
             * Full layout while keyboard is hidden.
             */
            else {

                draw_field(renderer,
                           sx, sy,
                           250, 55,
                           "PROBLEM TITLE",
                           title,
                           active == FIELD_TITLE);

                draw_field(renderer,
                           sx, sy,
                           350, 55,
                           "CAMPUS LOCATION",
                           location,
                           active == FIELD_LOCATION);

                draw_field(renderer,
                           sx, sy,
                           450, 110,
                           "DESCRIPTION",
                           description,
                           active == FIELD_DESCRIPTION);

                draw_text(renderer,
                          25 * sx,
                          610 * sy,
                          "CATEGORY (TAP TO CHANGE)",
                          70, 70, 70);

                draw_rect(renderer,
                          20 * sx,
                          620 * sy,
                          380 * sx,
                          55 * sy,
                          248, 243, 245);

                draw_text(renderer,
                          35 * sx,
                          642 * sy,
                          categories[category],
                          RED_R, RED_G, RED_B);

                draw_text(renderer,
                          25 * sx,
                          690 * sy,
                          "SEVERITY (TAP TO CHANGE)",
                          70, 70, 70);

                draw_rect(renderer,
                          20 * sx,
                          700 * sy,
                          380 * sx,
                          55 * sy,
                          248, 243, 245);

                draw_text(renderer,
                          35 * sx,
                          722 * sy,
                          severities[severity],
                          RED_R, RED_G, RED_B);

                if (error_message[0] != '\0') {

                    draw_text(renderer,
                              25 * sx,
                              775 * sy,
                              error_message,
                              200, 35, 35);
                }

                draw_button(renderer,
                            sx, sy,
                            790,
                            "SUBMIT REPORT");
            }
        }

        /* ---------------------------------------------
           CONFIRMATION SCREEN
           --------------------------------------------- */

        else if (screen == SCREEN_CONFIRMATION) {

            draw_rect(renderer,
                      0, 0,
                      output_w,
                      220 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer,
                      25 * sx,
                      65 * sy,
                      "PULSEMAP",
                      255, 255, 255);

            draw_text(renderer,
                      25 * sx,
                      110 * sy,
                      "REPORT VALIDATED",
                      255, 255, 255);

            draw_text(renderer,
                      25 * sx,
                      300 * sy,
                      "Your report passed validation.",
                      65, 65, 65);

            draw_text(renderer,
                      25 * sx,
                      345 * sy,
                      "Backend submission comes next.",
                      65, 65, 65);

            draw_button(renderer,
                        sx, sy,
                        470,
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
