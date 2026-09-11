#include "types.h"
#include "ui.h"
#include "browser.h"
#include "bookmarks.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define APP_TITLE "SwitchBrowser"
#define APP_VERSION "2.0.0"

// History
#define HISTORY_PATH "sdmc:/switch/SwitchBrowser/history.txt"
#define HISTORY_DIR  "sdmc:/switch/SwitchBrowser"

static void history_load(HistoryEntry* history, int* count, int max) {
    *count = 0;
    mkdir(HISTORY_DIR, 0777);
    FILE* f = fopen(HISTORY_PATH, "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f) && *count < max) {
            int len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[--len] = 0;
            if (len > 0) {
                strncpy(history[*count].url, line, MAX_URL_LEN - 1);
                history[*count].url[MAX_URL_LEN - 1] = 0;
                (*count)++;
            }
        }
        fclose(f);
    }
}

static void history_add(HistoryEntry* history, int* count, int max, const char* url) {
    if (*count >= max) {
        for (int i = 0; i < max - 1; i++)
            history[i] = history[i + 1];
        *count = max - 1;
    }
    strncpy(history[*count].url, url, MAX_URL_LEN - 1);
    history[*count].url[MAX_URL_LEN - 1] = 0;
    history[*count].timestamp = 0;
    (*count)++;

    FILE* f = fopen(HISTORY_PATH, "a");
    if (f) {
        fprintf(f, "%s\n", url);
        fclose(f);
    }
}

// --- Screen drawing ---

static void draw_home(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, APP_TITLE " v" APP_VERSION);

    // Search bar
    UIRect search_bar = {PADDING, TOP_BAR_H + 30, SCREEN_WIDTH - 2 * PADDING, 56};
    ui_input_box(ctx, search_bar, app->current_url, false);

    // Quick links grid (2x5)
    int grid_y = TOP_BAR_H + 120;
    int card_w = (SCREEN_WIDTH - 3 * PADDING) / 2;
    int card_h = 72;
    int cols = 2;

    const char* quick_links[] = {
        "Google", "https://www.google.com",
        "YouTube", "https://www.youtube.com",
        "Wikipedia", "https://www.wikipedia.org",
        "Bilibili", "https://www.bilibili.com",
        "Reddit", "https://www.reddit.com",
        "GitHub", "https://github.com",
        "Twitter", "https://twitter.com",
        "Amazon", "https://www.amazon.com",
    };
    int num_links = sizeof(quick_links) / sizeof(quick_links[0]) / 2;

    for (int i = 0; i < num_links; i++) {
        int col = i % cols;
        int row = i / cols;
        int x = PADDING + col * (card_w + PADDING);
        int y = grid_y + row * (card_h + 10);
        bool sel = (i == app->selected_idx);
        ui_card(ctx, (UIRect){x, y, card_w, card_h}, quick_links[i*2], quick_links[i*2+1], sel);
    }

    ui_draw_text(ctx, "Press A to search or open URL", PADDING, SCREEN_HEIGHT - BOT_BAR_H - 40, 14, COL_TEXT_DIM);
    ui_bottom_bar(ctx, 0);
    ui_present(ctx);
}

static void draw_bookmarks(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, "Bookmarks");

    if (app->bookmark_count == 0) {
        ui_draw_text_centered(ctx, "No bookmarks yet", 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, 24, COL_TEXT_DIM);
    } else {
        int card_w = SCREEN_WIDTH - 2 * PADDING;
        int card_h = 72;
        int y_start = TOP_BAR_H + 20;
        int visible = (SCREEN_HEIGHT - BOT_BAR_H - y_start) / (card_h + 10);

        for (int i = 0; i < visible && i + app->scroll_offset < app->bookmark_count; i++) {
            int idx = i + app->scroll_offset;
            bool sel = (idx == app->selected_idx);
            ui_card(ctx, (UIRect){PADDING, y_start + i * (card_h + 10), card_w, card_h},
                    app->bookmarks[idx].title, app->bookmarks[idx].url, sel);
        }
    }

    ui_draw_text(ctx, "A=Open  Y=Delete  X=Add current", PADDING, SCREEN_HEIGHT - BOT_BAR_H - 35, 14, COL_TEXT_DIM);
    ui_bottom_bar(ctx, 1);
    ui_present(ctx);
}

static void draw_history(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, "History");

    if (app->history_count == 0) {
        ui_draw_text_centered(ctx, "No history yet", 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, 24, COL_TEXT_DIM);
    } else {
        int card_w = SCREEN_WIDTH - 2 * PADDING;
        int card_h = 64;
        int y_start = TOP_BAR_H + 20;
        int visible = (SCREEN_HEIGHT - BOT_BAR_H - y_start) / (card_h + 10);

        for (int i = 0; i < visible && i + app->scroll_offset < app->history_count; i++) {
            int idx = i + app->scroll_offset;
            bool sel = (idx == app->selected_idx);
            ui_card(ctx, (UIRect){PADDING, y_start + i * (card_h + 10), card_w, card_h},
                    app->history[idx].url, "", sel);
        }
    }

    ui_draw_text(ctx, "A=Open  X=Clear  B=Back", PADDING, SCREEN_HEIGHT - BOT_BAR_H - 35, 14, COL_TEXT_DIM);
    ui_bottom_bar(ctx, 2);
    ui_present(ctx);
}

static void draw_settings(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, "Settings");

    int y = TOP_BAR_H + 20;
    int card_w = SCREEN_WIDTH - 2 * PADDING;

    ui_card(ctx, (UIRect){PADDING, y, card_w, 60}, "Homepage", app->config.homepage, app->selected_idx == 0);
    y += 70;
    ui_card(ctx, (UIRect){PADDING, y, card_w, 60}, "JavaScript", app->config.enable_js ? "Enabled" : "Disabled", app->selected_idx == 1);
    y += 70;
    ui_card(ctx, (UIRect){PADDING, y, card_w, 60}, "Touch", app->config.enable_touch ? "Enabled" : "Disabled", app->selected_idx == 2);
    y += 70;
    ui_card(ctx, (UIRect){PADDING, y, card_w, 60}, "Pointer", app->config.enable_pointer ? "Enabled" : "Disabled", app->selected_idx == 3);

    if (app->ui.applet_mode) {
        y += 80;
        ui_draw_text_wrapped(ctx,
            "WARNING: You are running in Applet mode.\n"
            "Web browsing requires Application mode.\n"
            "Hold R while launching a game, then select\n"
            "SwitchBrowser from hbmenu overlay.",
            PADDING, y, card_w, 14, COL_ERROR);
    }

    ui_draw_text(ctx, "A=Edit/Toggle  B=Back", PADDING, SCREEN_HEIGHT - BOT_BAR_H - 35, 14, COL_TEXT_DIM);
    ui_bottom_bar(ctx, 3);
    ui_present(ctx);
}

static void draw_applet_warning(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);

    // Center warning card
    int card_w = 800;
    int card_h = 360;
    int cx = (SCREEN_WIDTH - card_w) / 2;
    int cy = (SCREEN_HEIGHT - card_h) / 2;

    ui_fill_rounded_rect(ctx, cx, cy, card_w, card_h, 20, COL_CARD);
    ui_fill_rounded_rect(ctx, cx, cy, card_w, 4, 2, COL_ERROR);

    ui_draw_text_centered(ctx, "Applet Mode Detected", cx, cy + 30, card_w, 28, COL_ERROR);
    ui_draw_text_wrapped(ctx,
        "\nWeb browsing is not available in Applet mode.\n\n"
        "To use SwitchBrowser, please launch it via\n"
        "Title Override:\n\n"
        "1. Hold R while launching any game\n"
        "2. Select SwitchBrowser from hbmenu\n"
        "3. Press A to confirm\n\n"
        "Or use an NSP forwarder to launch directly.",
        cx + 30, cy + 70, card_w - 60, 18, COL_TEXT);

    ui_draw_text_centered(ctx, "Press + or B to exit", cx, cy + card_h - 50, card_w, 14, COL_TEXT_DIM);
    ui_present(ctx);
}

static void handle_home(AppContext* app, u64 key) {
    int num_items = 8;
    int cols = 2;
    int rows = (num_items + cols - 1) / cols;

    if (key & HidNpadButton_Down) {
        app->selected_idx++;
        if (app->selected_idx >= num_items) app->selected_idx = 0;
    }
    if (key & HidNpadButton_Up) {
        app->selected_idx--;
        if (app->selected_idx < 0) app->selected_idx = num_items - 1;
    }
    if (key & HidNpadButton_Right) {
        if (app->selected_idx % 2 == 0) app->selected_idx++;
        if (app->selected_idx >= num_items) app->selected_idx = 0;
    }
    if (key & HidNpadButton_Left) {
        if (app->selected_idx % 2 == 1) app->selected_idx--;
    }

    if (key & HidNpadButton_A) {
        app->state = STATE_URL_INPUT;
    }
    if (key & HidNpadButton_B) {
        app->exit_app = true;
    }
}

static void handle_url_input(AppContext* app) {
    char input[MAX_URL_LEN] = {0};

    if (browser_input_url(input, sizeof(input), "https://")) {
        if (browser_is_url(input)) {
            browser_normalize_url(input, sizeof(input));
            strncpy(app->current_url, input, MAX_URL_LEN - 1);
        } else {
            browser_build_search_url(app->current_url, MAX_URL_LEN,
                                     input, app->config.search_engine);
        }
        history_add(app->history, &app->history_count, MAX_HISTORY, app->current_url);
        app->state = STATE_BROWSING;
    } else {
        app->state = STATE_HOME;
    }
}

static void handle_browsing(AppContext* app) {
    if (app->ui.applet_mode) {
        // Can't launch web applet in applet mode
        app->state = STATE_HOME;
        return;
    }

    char last_url[MAX_URL_LEN] = {0};
    if (browser_navigate(app->current_url, last_url, sizeof(last_url))) {
        if (last_url[0]) {
            strncpy(app->last_url, last_url, MAX_URL_LEN - 1);
            history_add(app->history, &app->history_count, MAX_HISTORY, last_url);
        }
    }
    app->state = STATE_HOME;
}

static void handle_bookmarks(AppContext* app, u64 key) {
    if (app->bookmark_count == 0) {
        if (key & (HidNpadButton_B | HidNpadButton_Plus)) {
            app->state = STATE_HOME;
            app->selected_idx = 0;
        }
        return;
    }

    if (key & HidNpadButton_Down) {
        if (app->selected_idx < app->bookmark_count - 1) {
            app->selected_idx++;
            if (app->selected_idx >= app->scroll_offset + 7)
                app->scroll_offset = app->selected_idx - 6;
        }
    }
    if (key & HidNpadButton_Up) {
        if (app->selected_idx > 0) {
            app->selected_idx--;
            if (app->selected_idx < app->scroll_offset)
                app->scroll_offset = app->selected_idx;
        }
    }
    if (key & HidNpadButton_A) {
        strncpy(app->current_url, app->bookmarks[app->selected_idx].url, MAX_URL_LEN - 1);
        history_add(app->history, &app->history_count, MAX_HISTORY, app->current_url);
        app->state = STATE_BROWSING;
    }
    if (key & HidNpadButton_Y) {
        bookmarks_remove(app->bookmarks, &app->bookmark_count, app->selected_idx);
        if (app->selected_idx >= app->bookmark_count && app->selected_idx > 0)
            app->selected_idx--;
    }
    if (key & HidNpadButton_X && app->last_url[0]) {
        bookmarks_add(app->bookmarks, &app->bookmark_count, MAX_BOOKMARKS, app->last_url, "");
    }
    if (key & HidNpadButton_B) {
        app->state = STATE_HOME;
        app->selected_idx = 0;
    }
}

static void handle_history(AppContext* app, u64 key) {
    if (app->history_count == 0) {
        if (key & (HidNpadButton_B | HidNpadButton_Plus)) {
            app->state = STATE_HOME;
            app->selected_idx = 0;
        }
        return;
    }

    if (key & HidNpadButton_Down) {
        if (app->selected_idx < app->history_count - 1) {
            app->selected_idx++;
            if (app->selected_idx >= app->scroll_offset + 7)
                app->scroll_offset = app->selected_idx - 6;
        }
    }
    if (key & HidNpadButton_Up) {
        if (app->selected_idx > 0) {
            app->selected_idx--;
            if (app->selected_idx < app->scroll_offset)
                app->scroll_offset = app->selected_idx;
        }
    }
    if (key & HidNpadButton_A) {
        strncpy(app->current_url, app->history[app->selected_idx].url, MAX_URL_LEN - 1);
        app->state = STATE_BROWSING;
    }
    if (key & HidNpadButton_X) {
        app->history_count = 0;
        FILE* f = fopen(HISTORY_PATH, "w");
        if (f) fclose(f);
    }
    if (key & HidNpadButton_B) {
        app->state = STATE_HOME;
        app->selected_idx = 0;
    }
}

static void handle_settings(AppContext* app, u64 key) {
    if (key & HidNpadButton_Down) {
        app->selected_idx = (app->selected_idx + 1) % 4;
    }
    if (key & HidNpadButton_Up) {
        app->selected_idx = (app->selected_idx + 3) % 4;
    }
    if (key & HidNpadButton_A) {
        char input[MAX_URL_LEN];
        switch (app->selected_idx) {
            case 0:
                if (browser_input_text(input, sizeof(input), "Edit Homepage", app->config.homepage)) {
                    strncpy(app->config.homepage, input, MAX_URL_LEN - 1);
                }
                break;
            case 1: app->config.enable_js = !app->config.enable_js; break;
            case 2: app->config.enable_touch = !app->config.enable_touch; break;
            case 3: app->config.enable_pointer = !app->config.enable_pointer; break;
        }
    }
    if (key & HidNpadButton_B) {
        app->state = STATE_HOME;
        app->selected_idx = 0;
    }
}

int main(int argc, char* argv[]) {
    AppContext app;
    memset(&app, 0, sizeof(app));

    if (!ui_init(&app.ui)) {
        consoleInit(NULL);
        printf("Failed to initialize UI!\n");
        consoleUpdate(NULL);
        svcSleepThread(3000000000);
        consoleExit(NULL);
        return 1;
    }

    app.state = STATE_HOME;
    app.selected_idx = 0;
    app.scroll_offset = 0;

    // Load config
    strncpy(app.config.homepage, "https://www.google.com", MAX_URL_LEN - 1);
    strncpy(app.config.search_engine, "https://www.google.com/search?q=", MAX_URL_LEN - 1);
    app.config.enable_js = true;
    app.config.enable_touch = true;
    app.config.enable_pointer = true;

    bookmarks_load(app.bookmarks, &app.bookmark_count, MAX_BOOKMARKS);
    history_load(app.history, &app.history_count, MAX_HISTORY);

    bool show_applet_warning = app.ui.applet_mode;

    while (appletMainLoop() && !app.exit_app) {
        u64 key = pad_get_keys(&app.ui.pad);

        // Check for app toggle
        if (key & HidNpadButton_Plus) {
            if (app.state == STATE_HOME || show_applet_warning) {
                app.exit_app = true;
                break;
            }
        }

        // Handle states that need SDL exit
        if (app.state == STATE_URL_INPUT) {
            SDL_ShowCursor(SDL_ENABLE);
            SDL_StopTextInput();
            // Exit SDL rendering temporarily for swkbd
            SDL_DestroyRenderer(app.ui.renderer);
            SDL_DestroyWindow(app.ui.window);

            handle_url_input(&app);

            // Recreate SDL window
            SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN,
                                       &app.ui.window, &app.ui.renderer);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            continue;
        }

        if (app.state == STATE_BROWSING) {
            // Exit SDL for web applet
            SDL_DestroyRenderer(app.ui.renderer);
            SDL_DestroyWindow(app.ui.window);

            handle_browsing(&app);

            // Recreate SDL window
            SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN,
                                       &app.ui.window, &app.ui.renderer);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            continue;
        }

        // Show applet warning on first launch in applet mode
        if (show_applet_warning && app.state == STATE_HOME) {
            draw_applet_warning(&app);
            if (key & (HidNpadButton_B | HidNpadButton_Plus | HidNpadButton_A)) {
                show_applet_warning = false;
            }
        } else {
            switch (app.state) {
                case STATE_HOME:
                    handle_home(&app, key);
                    draw_home(&app);
                    break;
                case STATE_BOOKMARKS:
                    handle_bookmarks(&app, key);
                    draw_bookmarks(&app);
                    break;
                case STATE_HISTORY:
                    handle_history(&app, key);
                    draw_history(&app);
                    break;
                case STATE_SETTINGS:
                    handle_settings(&app, key);
                    draw_settings(&app);
                    break;
                default:
                    app.state = STATE_HOME;
                    break;
            }
        }

        // Bottom tab navigation via touch zones
        // Left/right on bottom bar switches tabs
        if (key & HidNpadButton_R && app.state != STATE_BROWSING) {
            // Cycle through tabs
            int states[] = {STATE_HOME, STATE_BOOKMARKS, STATE_HISTORY, STATE_SETTINGS};
            int cur = 0;
            for (int i = 0; i < 4; i++) if (states[i] == app.state) cur = i;
            app.state = states[(cur + 1) % 4];
            app.selected_idx = 0;
            app.scroll_offset = 0;
        }
    }

    ui_exit(&app.ui);
    return 0;
}
