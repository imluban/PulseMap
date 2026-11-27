#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

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
    SCREEN_INCIDENTS,
    SCREEN_CONFIRMATION
} AppScreen;

typedef struct {
    char title[96];
    char location[96];
    char description[256];
    int category;
    int severity;
} ReportRecord;

#define MAX_REPORTS 100
#define REPORT_FILE_NAME "reports.dat"

static ReportRecord reports[MAX_REPORTS];
static size_t report_count = 0;

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

static float ui_scale = 1.0f;

static void draw_text(SDL_Renderer *renderer,
                      float x, float y,
                      const char *text,
                      Uint8 red, Uint8 green, Uint8 blue)
{
    static const unsigned char font[36][7] = {
        {14,17,17,31,17,17,17}, /* A */
        {30,17,17,30,17,17,30}, /* B */
        {14,17,16,16,16,17,14}, /* C */
        {30,17,17,17,17,17,30}, /* D */
        {31,16,16,30,16,16,31}, /* E */
        {31,16,16,30,16,16,16}, /* F */
        {14,17,16,23,17,17,15}, /* G */
        {17,17,17,31,17,17,17}, /* H */
        {14,4,4,4,4,4,14},      /* I */
        {7,2,2,2,18,18,12},     /* J */
        {17,18,20,24,20,18,17}, /* K */
        {16,16,16,16,16,16,31}, /* L */
        {17,27,21,21,17,17,17}, /* M */
        {17,25,21,19,17,17,17}, /* N */
        {14,17,17,17,17,17,14}, /* O */
        {30,17,17,30,16,16,16}, /* P */
        {14,17,17,17,21,18,13}, /* Q */
        {30,17,17,30,20,18,17}, /* R */
        {15,16,16,14,1,1,30},   /* S */
        {31,4,4,4,4,4,4},       /* T */
        {17,17,17,17,17,17,14}, /* U */
        {17,17,17,17,17,10,4},  /* V */
        {17,17,17,21,21,21,10}, /* W */
        {17,17,10,4,10,17,17},  /* X */
        {17,17,10,4,4,4,4},     /* Y */
        {31,1,2,4,8,16,31},     /* Z */

        {14,17,19,21,25,17,14}, /* 0 */
        {4,12,4,4,4,4,14},      /* 1 */
        {14,17,1,2,4,8,31},     /* 2 */
        {30,1,1,14,1,1,30},     /* 3 */
        {2,6,10,18,31,2,2},     /* 4 */
        {31,16,16,30,1,1,30},   /* 5 */
        {14,16,16,30,17,17,14}, /* 6 */
        {31,1,2,4,8,8,8},       /* 7 */
        {14,17,17,14,17,17,14}, /* 8 */
        {14,17,17,15,1,1,14}    /* 9 */
    };

    float scale = 2.0f * ui_scale;
    float available = (420.0f - x / ui_scale - 12.0f) * ui_scale;
    size_t length = strlen(text);

    if (length > 0 && length * 6.0f * scale > available) {
        scale = available / (length * 6.0f);
    }

    if (scale < 1.25f * ui_scale) {
        scale = 1.25f * ui_scale;
    }

    SDL_SetRenderDrawColor(renderer, red, green, blue, 255);

    float cursor = x;

    for (size_t i = 0; i < length; i++) {
        unsigned char c = (unsigned char)text[i];
        unsigned char upper = (unsigned char)toupper(c);
        const unsigned char *glyph = NULL;

        if (upper >= 'A' && upper <= 'Z') {
            glyph = font[upper - 'A'];
        }
        else if (upper >= '0' && upper <= '9') {
            glyph = font[26 + upper - '0'];
        }

        if (glyph != NULL) {
            for (int row = 0; row < 7; row++) {
                for (int col = 0; col < 5; col++) {
                    if (glyph[row] & (1 << (4 - col))) {
                        SDL_FRect pixel = {
                            cursor + col * scale,
                            y + row * scale,
                            scale,
                            scale
                        };

                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
        }
        else {
            /* Basic punctuation */
            int row = 6;
            int col = 2;

            if (c == '.') {
                row = 6;
            }
            else if (c == ',') {
                row = 6;
            }
            else if (c == '-') {
                row = 3;
            }
            else if (c == ':') {
                row = 2;
            }
            else if (c == '/') {
                row = 3;
                col = 3;
            }
            else if (c == '<') {
                row = 3;
                col = 1;
            }
            else if (c == '>') {
                row = 3;
                col = 3;
            }
            else if (c == '\'') {
                row = 0;
                col = 2;
            }
            else if (c == ' ') {
                cursor += 6 * scale;
                continue;
            }

            if (c != ' ') {
                SDL_FRect pixel = {
                    cursor + col * scale,
                    y + row * scale,
                    scale,
                    scale
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }

        cursor += 6 * scale;
    }
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
 * SDL_TEXTINPUT contains newly committed text. Append it to the
 * currently active field; do not infer cumulative text from its contents.
 */
static void process_text_input(ActiveField field, const char *input)
{
    char *buffer = get_active_buffer(field);
    size_t capacity = get_active_capacity(field);

    if (buffer == NULL || input == NULL || capacity == 0) {
        return;
    }

    size_t current_length = strlen(buffer);
    size_t incoming_length = strlen(input);

    if (incoming_length == 0 || current_length >= capacity) {
        return;
    }

    /* Leave room for the terminating null byte. */
    if (incoming_length > capacity - current_length - 1) {
        return;
    }

    memcpy(buffer + current_length, input, incoming_length + 1);
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
 * Change the active field without ending the Android IME session.
 * Starting text input is only necessary when entering the first field.
 */
static void activate_field(SDL_Window *window,
                           ActiveField *active,
                           ActiveField next)
{
    if (*active == next) {
        return;
    }

    if (*active == FIELD_NONE) {
        SDL_StartTextInput(window);
    }

    *active = next;
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

/* Store reports in SDL's private, persistent application directory. */
static bool save_reports(void)
{
    char *directory = SDL_GetPrefPath("EastDelta", "PulseMap");
    char path[1024];
    FILE *file;
    bool success = false;

    if (directory == NULL) {
        return false;
    }

    if (snprintf(path, sizeof(path), "%s%s", directory,
                 REPORT_FILE_NAME) >= (int)sizeof(path)) {
        SDL_free(directory);
        return false;
    }

    file = fopen(path, "wb");
    if (file != NULL) {
        uint32_t count = (uint32_t)report_count;
        if (fwrite(&count, sizeof(count), 1, file) == 1 &&
            (count == 0 || fwrite(reports, sizeof(ReportRecord), count, file) == count)) {
            success = true;
        }
        fclose(file);
    }

    SDL_free(directory);
    return success;
}

static void load_reports(void)
{
    char *directory = SDL_GetPrefPath("EastDelta", "PulseMap");
    char path[1024];
    FILE *file;
    uint32_t count = 0;

    report_count = 0;
    if (directory == NULL) {
        return;
    }

    if (snprintf(path, sizeof(path), "%s%s", directory,
                 REPORT_FILE_NAME) >= (int)sizeof(path)) {
        SDL_free(directory);
        return;
    }

    file = fopen(path, "rb");
    if (file != NULL) {
        if (fread(&count, sizeof(count), 1, file) == 1 &&
            count <= MAX_REPORTS &&
            (count == 0 || fread(reports, sizeof(ReportRecord), count, file) == count)) {
            report_count = count;
        }
        fclose(file);
    }

    SDL_free(directory);
}

static bool submit_report(AppScreen *screen)
{
    ReportRecord record;

    if (title[0] == '\0') {
        snprintf(error_message, sizeof(error_message),
                 "Please enter a problem title.");
        return false;
    }

    if (location[0] == '\0') {
        snprintf(error_message, sizeof(error_message),
                 "Please enter a campus location.");
        return false;
    }

    if (description[0] == '\0') {
        snprintf(error_message, sizeof(error_message),
                 "Please describe the problem.");
        return false;
    }

    memset(&record, 0, sizeof(record));

    snprintf(record.title, sizeof(record.title), "%s", title);
    snprintf(record.location, sizeof(record.location), "%s", location);
    snprintf(record.description, sizeof(record.description), "%s", description);

    record.category = category;
    record.severity = severity;

    /* Keep the newest MAX_REPORTS entries. */
    if (report_count == MAX_REPORTS) {
        memmove(&reports[0], &reports[1],
                (MAX_REPORTS - 1) * sizeof(ReportRecord));
        report_count--;
    }

    reports[report_count++] = record;

    if (!save_reports()) {
        report_count--;

        snprintf(error_message, sizeof(error_message),
                 "Could not save report on this device.");

        return false;
    }

    error_message[0] = '\0';
    *screen = SCREEN_CONFIRMATION;

    return true;
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

    load_reports();

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
        ui_scale = (sx < sy) ? sx : sy;

        keyboard_visible = SDL_ScreenKeyboardShown(window);

        /* -------------------------------------------------
           EVENT HANDLING
           ------------------------------------------------- */

        while (SDL_PollEvent(&event)) {

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }

            /* Text committed by the Android keyboard. */
            if (event.type == SDL_EVENT_TEXT_INPUT &&
                active != FIELD_NONE) {

                process_text_input(active, event.text.text);
                continue;
            }

            /* Backspace. */
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.key == SDLK_BACKSPACE &&
                active != FIELD_NONE) {

                char *buffer = get_active_buffer(active);

                if (buffer != NULL) {
                    remove_last_character(buffer);
                }

                continue;
            }

            /* Convert touch or mouse coordinates. */
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

            /* DASHBOARD */
            if (screen == SCREEN_DASHBOARD) {

                if (tx >= 20 && tx <= 400 &&
                    ty >= 545 && ty <= 620) {

                    screen = SCREEN_REPORT;
                    error_message[0] = '\0';
                }
                else if (tx >= 20 && tx <= 400 &&
                         ty >= 645 && ty <= 711) {

                    screen = SCREEN_INCIDENTS;
                }
            }

            /* REPORT FORM */
            else if (screen == SCREEN_REPORT) {

                /* Back button. */
                if (ty >= 40 && ty <= 100 &&
                    tx >= 20 && tx <= 150) {

                    stop_text_input(window, &active);
                    screen = SCREEN_DASHBOARD;
                    continue;
                }

                /* Done button. */
                if (keyboard_visible &&
                    ty >= 30 && ty <= 90 &&
                    tx >= 315 && tx <= 405) {

                    stop_text_input(window, &active);
                    continue;
                }

                /* Compact layout while keyboard is visible. */
                if (keyboard_visible) {

                    if (ty >= 155 && ty <= 245) {
                        activate_field(window, &active, FIELD_TITLE);
                    }
                    else if (ty >= 250 && ty <= 345) {
                        activate_field(window, &active, FIELD_LOCATION);
                    }
                    else if (ty >= 350 && ty <= 485) {
                        activate_field(window, &active, FIELD_DESCRIPTION);
                    }
                }

                /* Normal layout while keyboard is hidden. */
                else {

                    if (ty >= 250 && ty <= 325) {
                        activate_field(window, &active, FIELD_TITLE);
                    }
                    else if (ty >= 350 && ty <= 425) {
                        activate_field(window, &active, FIELD_LOCATION);
                    }
                    else if (ty >= 450 && ty <= 590) {
                        activate_field(window, &active, FIELD_DESCRIPTION);
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

            /* PROBLEM LIST */
            else if (screen == SCREEN_INCIDENTS) {

                if (ty >= 35 && ty <= 115 &&
                    tx >= 15 && tx <= 180) {

                    screen = SCREEN_DASHBOARD;
                }
                else if (report_count == 0 &&
                         ty >= 470 && ty <= 545) {

                    screen = SCREEN_REPORT;
                    error_message[0] = '\0';
                }
            }

            /* CONFIRMATION */
            else if (screen == SCREEN_CONFIRMATION) {

                if (ty >= 470 && ty <= 550) {
                    clear_report();
                    screen = SCREEN_DASHBOARD;
                }
                else if (ty >= 570 && ty <= 650) {
                    clear_report();
                    screen = SCREEN_INCIDENTS;
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

        /* DASHBOARD SCREEN */
        if (screen == SCREEN_DASHBOARD) {

            /* Brand header */
            draw_rect(renderer, 0, 0, output_w, 225 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 24 * sx, 42 * sy,
                      "PULSEMAP", 255, 255, 255);

            draw_text(renderer, 24 * sx, 75 * sy,
                      "EAST DELTA UNIVERSITY", 245, 220, 228);

            draw_text(renderer, 24 * sx, 132 * sy,
                      "Your campus. Your voice.", 255, 255, 255);

            draw_text(renderer, 24 * sx, 165 * sy,
                      "Spot it. Report it. Improve it.", 245, 220, 228);

            /* Overview panel */
            draw_rect(renderer, 16 * sx, 245 * sy, 388 * sx, 118 * sy,
                      248, 243, 245);

            draw_text(renderer, 32 * sx, 270 * sy,
                      "YOUR CAMPUS AT A GLANCE", RED_R, RED_G, RED_B);

            {
                char count_text[48];

                snprintf(count_text, sizeof(count_text),
                         "%u", (unsigned)report_count);

                draw_text(renderer, 34 * sx, 315 * sy,
                          count_text, 45, 35, 40);

                draw_text(renderer, 76 * sx, 315 * sy,
                          "LOCAL REPORTS", 90, 75, 80);

                draw_text(renderer, 250 * sx, 315 * sy,
                          "ON THIS DEVICE", 90, 75, 80);
            }

            /* Problem radar */
            draw_rect(renderer, 16 * sx, 385 * sy, 388 * sx, 125 * sy,
                      255, 255, 255);

            draw_rect(renderer, 16 * sx, 385 * sy, 5 * sx, 125 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 32 * sx, 410 * sy,
                      "CAMPUS PROBLEM RADAR", RED_R, RED_G, RED_B);

            draw_text(renderer, 32 * sx, 447 * sy,
                      "Emerging campus issues", 45, 45, 45);

            draw_text(renderer, 32 * sx, 475 * sy,
                      report_count
                          ? "Your submitted reports are being tracked."
                          : "No reports yet. Be the first to speak up.",
                      105, 95, 100);

            /* Primary actions */
            draw_button(renderer, sx, sy, 545, "REPORT A PROBLEM");

            draw_rect(renderer, 20 * sx, 645 * sy, 380 * sx, 66 * sy,
                      248, 243, 245);

            draw_text(renderer, 112 * sx, 674 * sy,
                      "VIEW PROBLEM LIST", RED_R, RED_G, RED_B);

            draw_text(renderer, 24 * sx, 765 * sy,
                      "FEEL THE CAMPUS. DRIVE THE CHANGE.",
                      115, 95, 102);

            draw_text(renderer, 24 * sx, 815 * sy,
                      "PULSEMAP  /  EAST DELTA", 145, 125, 132);
        }

        /* REPORT SCREEN */
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

            /* Compact layout while keyboard is visible. */
            if (keyboard_visible) {

                draw_done_button(renderer, sx, sy);

                draw_field(renderer, sx, sy,
                           180, 55,
                           "PROBLEM TITLE",
                           title,
                           active == FIELD_TITLE);

                draw_field(renderer, sx, sy,
                           275, 55,
                           "CAMPUS LOCATION",
                           location,
                           active == FIELD_LOCATION);

                draw_field(renderer, sx, sy,
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

            /* Full layout while keyboard is hidden. */
            else {

                draw_field(renderer, sx, sy,
                           250, 55,
                           "PROBLEM TITLE",
                           title,
                           active == FIELD_TITLE);

                draw_field(renderer, sx, sy,
                           350, 55,
                           "CAMPUS LOCATION",
                           location,
                           active == FIELD_LOCATION);

                draw_field(renderer, sx, sy,
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

        /* PROBLEM LIST SCREEN */
        else if (screen == SCREEN_INCIDENTS) {

            draw_rect(renderer, 0, 0, output_w, 165 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 24 * sx, 48 * sy,
                      "< BACK", 255, 255, 255);

            draw_text(renderer, 24 * sx, 100 * sy,
                      "PROBLEM LIST", 255, 255, 255);

            draw_text(renderer, 24 * sx, 133 * sy,
                      "Campus reports saved on this device",
                      245, 220, 228);

            if (report_count == 0) {

                draw_rect(renderer, 18 * sx, 220 * sy,
                          384 * sx, 190 * sy,
                          248, 243, 245);

                draw_text(renderer, 38 * sx, 265 * sy,
                          "ALL QUIET FOR NOW",
                          RED_R, RED_G, RED_B);

                draw_text(renderer, 38 * sx, 310 * sy,
                          "No reports have been submitted yet.",
                          75, 75, 75);

                draw_text(renderer, 38 * sx, 350 * sy,
                          "Help improve campus by reporting an issue.",
                          105, 95, 100);

                draw_button(renderer, sx, sy,
                            470, "REPORT A PROBLEM");

            } else {

                char summary[96];

                unsigned shown =
                    (report_count < 5)
                        ? (unsigned)report_count
                        : 5U;

                snprintf(summary, sizeof(summary),
                         "%u RECENT REPORT(S)",
                         (unsigned)report_count);

                draw_text(renderer, 22 * sx, 195 * sy,
                          summary, RED_R, RED_G, RED_B);

                for (unsigned i = 0; i < shown; ++i) {

                    size_t index = report_count - 1U - i;
                    float y = 225.0f + (float)i * 125.0f;

                    char meta[128];
                    char detail[80];

                    draw_rect(renderer,
                              18 * sx, y * sy,
                              384 * sx, 108 * sy,
                              248, 243, 245);

                    draw_rect(renderer,
                              18 * sx, y * sy,
                              4 * sx, 108 * sy,
                              RED_R, RED_G, RED_B);

                    draw_text(renderer,
                              32 * sx, (y + 17) * sy,
                              reports[index].title,
                              65, 30, 42);

                    snprintf(meta, sizeof(meta),
                             "%s  /  %s",
                             (reports[index].category >= 0 &&
                              reports[index].category < 5)
                                 ? categories[reports[index].category]
                                 : "Other",
                             (reports[index].severity >= 0 &&
                              reports[index].severity < 4)
                                 ? severities[reports[index].severity]
                                 : "Medium");

                    draw_text(renderer,
                              32 * sx, (y + 47) * sy,
                              meta,
                              RED_R, RED_G, RED_B);

                    snprintf(detail, sizeof(detail),
                             "%.25s  -  %.35s",
                             reports[index].location,
                             reports[index].description);

                    draw_text(renderer,
                              32 * sx, (y + 78) * sy,
                              detail,
                              100, 90, 95);
                }
            }
        }

        /* CONFIRMATION SCREEN */
        else if (screen == SCREEN_CONFIRMATION) {

            draw_rect(renderer, 0, 0, output_w, 225 * sy,
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 24 * sx, 55 * sy,
                      "PULSEMAP", 255, 255, 255);

            draw_text(renderer, 24 * sx, 112 * sy,
                      "REPORT RECEIVED", 255, 255, 255);

            draw_rect(renderer, 20 * sx, 275 * sy,
                      380 * sx, 150 * sy,
                      248, 243, 245);

            draw_text(renderer, 40 * sx, 315 * sy,
                      "THANK YOU FOR SPEAKING UP",
                      RED_R, RED_G, RED_B);

            draw_text(renderer, 40 * sx, 360 * sy,
                      "Your report is saved on this device.",
                      65, 65, 65);

            draw_button(renderer, sx, sy,
                        470, "RETURN TO DASHBOARD");

            draw_button(renderer, sx, sy,
                        570, "VIEW PROBLEM LIST");
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
