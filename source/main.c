#include "types.h"
#include "ui.h"
#include "browser.h"
#include "web_fetch.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define APP_TITLE "SwitchBrowser"
#define APP_VERSION "2.1.0"

typedef struct { const char* name; const char* url; } QuickLink;

static const QuickLink quick_links[] = {
    {"Baidu",       "https://www.baidu.com"},
    {"Bing",        "https://www.bing.com"},
    {"Google",      "https://www.google.com"},
    {"GitHub",      "https://github.com"},
    {"YouTube",     "https://www.youtube.com"},
    {"Wikipedia",   "https://en.wikipedia.org"},
    {"Reddit",      "https://www.reddit.com"},
    {"Hacker News", "https://news.ycombinator.com"},
};
#define QUICK_LINK_COUNT (sizeof(quick_links) / sizeof(quick_links[0]))

#define BOOKMARK_PATH "sdmc:/switch/SwitchBrowser/bookmarks.txt"
#define BOOKMARK_DIR  "sdmc:/switch/SwitchBrowser"

static void bookmarks_load(AppContext* app) {
    app->bookmark_count = 0;
    mkdir(BOOKMARK_DIR, 0777);
    FILE* f = fopen(BOOKMARK_PATH, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f) && app->bookmark_count < MAX_BOOKMARKS) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = 0;
        if (len > 0 && line[len-1] == '\r') line[--len] = 0;
        if (!len) continue;
        char* sep = strchr(line, '|');
        if (sep) {
            *sep = 0;
            strncpy(app->bookmarks[app->bookmark_count].title, line, MAX_TEXT_LEN - 1);
            strncpy(app->bookmarks[app->bookmark_count].url, sep + 1, MAX_URL_LEN - 1);
            app->bookmark_count++;
        }
    }
    fclose(f);
}

static void bookmarks_save(AppContext* app) {
    mkdir(BOOKMARK_DIR, 0777);
    FILE* f = fopen(BOOKMARK_PATH, "w");
    if (!f) return;
    for (int i = 0; i < app->bookmark_count; i++) {
        fprintf(f, "%s|%s\n", app->bookmarks[i].title, app->bookmarks[i].url);
    }
    fclose(f);
}

static void draw_home(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, APP_TITLE " v" APP_VERSION, app->ui.applet_mode ? "[Applet Mode]" : NULL);

    UIRect search_bar = {PADDING, TOP_BAR_H + 30, SCREEN_WIDTH - 2 * PADDING, 56};
    ui_input_box(ctx, search_bar, app->current_url, false);

    int grid_y = TOP_BAR_H + 120;
    int card_w = (SCREEN_WIDTH - 3 * PADDING) / 2;
    int card_h = 72;

    for (int i = 0; i < (int)QUICK_LINK_COUNT && i < 8; i++) {
        int col = i % 2;
        int row = i / 2;
        UIRect r = {PADDING + col * (card_w + PADDING), grid_y + row * (card_h + 12), card_w, card_h};
        ui_quick_link_card(ctx, r, quick_links[i].name, quick_links[i].url, (i == app->selected_idx));
    }

    ui_bottom_bar(ctx, "A=Open  X=Search  Y=Bookmarks  Plus=Exit");
    ui_present(ctx);
}

static void draw_loading(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, "Loading", app->current_url);

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    static int frame = 0;
    frame++;
    set_color(ctx, COL_ACCENT);
    for (int i = 0; i < 8; i++) {
        float angle = (frame * 5 + i * 45) * 3.14159f / 180.0f;
        int r = 30 + (i * 4);
        int x = cx + (int)(r * cosf(angle));
        int y = cy + (int)(r * sinf(angle));
        int alpha = 255 - (i * 28);
        SDL_SetRenderDrawColor(ctx->renderer, 0x4F, 0xC3, 0xF7, alpha);
        SDL_Rect dot = {x - 4, y - 4, 8, 8};
        SDL_RenderFillRect(ctx->renderer, &dot);
    }

    ui_draw_text_centered(ctx, "Fetching page...", 0, cy + 80, SCREEN_WIDTH, 18, COL_TEXT_DIM);
    ui_draw_text_centered(ctx, app->current_url, 0, cy + 110, SCREEN_WIDTH, 14, COL_TEXT_DIM);
    ui_present(ctx);
}

static void draw_page(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, app->current_title[0] ? app->current_title : "Page", app->current_url);

    int y_start = TOP_BAR_H + 16;
    int max_w = SCREEN_WIDTH - 2 * PADDING;
    int line_h = 24;
    int visible_lines = (SCREEN_HEIGHT - BOT_BAR_H - y_start - 10) / line_h;

    if (app->page.line_count == 0) {
        ui_draw_text_centered(ctx, app->page.error_msg[0] ? app->page.error_msg : "Empty page",
                              0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, 18, COL_TEXT_DIM);
    } else {
        for (int i = 0; i < visible_lines && i + app->scroll_offset < app->page.line_count; i++) {
            int idx = i + app->scroll_offset;
            PageLine* line = &app->page.lines[idx];
            int y = y_start + i * line_h;

            if (line->link_idx >= 0) {
                u32 bg = (idx == app->selected_link + app->scroll_offset) ? COL_CARD_HL : COL_BG;
                if (idx == app->selected_link + app->scroll_offset) {
                    ui_fill_rect(ctx, PADDING, y, max_w, line_h, COL_CARD_HL);
                }
                ui_draw_text_wrapped(ctx, line->text, PADDING, y, max_w, 18, COL_LINK);
                int tw = ui_text_width(ctx, line->text, 18);
                set_color(ctx, COL_LINK);
                SDL_Rect underline = {PADDING, y + 20, tw, 1};
                SDL_RenderFillRect(ctx->renderer, &underline);
            } else {
                ui_draw_text_wrapped(ctx, line->text, PADDING, y, max_w, 18, COL_TEXT);
            }
        }
    }

    char info[128];
    snprintf(info, sizeof(info), "%d lines | HTTP %d", app->page.line_count, app->page.http_status);
    ui_bottom_bar(ctx, "A=Open Link  B=Back  Up/Down=Scroll  L/R=Page");
    ui_present(ctx);
}

static void draw_bookmarks(AppContext* app) {
    UIContext* ctx = &app->ui;
    ui_clear(ctx, COL_BG);
    ui_top_bar(ctx, "Bookmarks", NULL);

    int y_start = TOP_BAR_H + 20;
    int card_w = SCREEN_WIDTH - 2 * PADDING;
    int card_h = 60;

    if (app->bookmark_count == 0) {
        ui_draw_text_centered(ctx, "No bookmarks yet", 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, 24, COL_TEXT_DIM);
    } else {
        for (int i = 0; i < app->bookmark_count; i++) {
            UIRect r = {PADDING, y_start + i * (card_h + 8), card_w, card_h};
            ui_quick_link_card(ctx, r, app->bookmarks[i].title, app->bookmarks[i].url, (i == app->selected_idx));
        }
    }
    ui_bottom_bar(ctx, "A=Open  B=Back");
    ui_present(ctx);
}

static void fetch_page(AppContext* app, const char* url) {
    strncpy(app->current_url, url, MAX_URL_LEN - 1);
    app->current_url[MAX_URL_LEN - 1] = 0;
    app->state = STATE_LOADING;

    memset(&app->page, 0, sizeof(app->page));
    bool ok = web_fetch_page(url, &app->page);

    if (ok) {
        strncpy(app->current_title, app->page.title, MAX_TEXT_LEN - 1);
        app->current_title[MAX_TEXT_LEN - 1] = 0;
        app->state = STATE_PAGE_VIEW;
        app->scroll_offset = 0;
        app->selected_link = 0;
    } else {
        app->state = STATE_ERROR;
    }
}

static void handle_home(AppContext* app, u64 key) {
    if (key & HidNpadButton_Down) {
        if (app->selected_idx < (int)QUICK_LINK_COUNT - 1) app->selected_idx++;
    }
    if (key & HidNpadButton_Up) {
        if (app->selected_idx > 0) app->selected_idx--;
    }
    if (key & HidNpadButton_Left) {
        if (app->selected_idx >= 2) app->selected_idx -= 2;
    }
    if (key & HidNpadButton_Right) {
        if (app->selected_idx < (int)QUICK_LINK_COUNT - 2) app->selected_idx += 2;
    }
    if (key & HidNpadButton_A) {
        fetch_page(app, quick_links[app->selected_idx].url);
    }
    if (key & HidNpadButton_X) {
        char input[256];
        if (browser_input_url(input, sizeof(input), NULL)) {
            if (browser_is_url(input)) {
                browser_normalize_url(input, sizeof(input));
                fetch_page(app, input);
            } else {
                char search_url[MAX_URL_LEN];
                browser_build_search_url(search_url, sizeof(search_url), input, NULL);
                fetch_page(app, search_url);
            }
        }
    }
    if (key & HidNpadButton_Y) {
        app->prev_state = STATE_HOME;
        app->state = STATE_BOOKMARKS;
        app->selected_idx = 0;
    }
}

static void handle_page_view(AppContext* app, u64 key) {
    if (key & HidNpadButton_Down) {
        if (app->scroll_offset < app->page.line_count - 1) app->scroll_offset++;
    }
    if (key & HidNpadButton_Up) {
        if (app->scroll_offset > 0) app->scroll_offset--;
    }
    if (key & HidNpadButton_R) {
        app->scroll_offset += 20;
        if (app->scroll_offset >= app->page.line_count) app->scroll_offset = app->page.line_count - 1;
    }
    if (key & HidNpadButton_L) {
        app->scroll_offset -= 20;
        if (app->scroll_offset < 0) app->scroll_offset = 0;
    }
    if (key & HidNpadButton_A) {
        // Find next link to navigate
        for (int i = app->scroll_offset; i < app->page.line_count; i++) {
            if (app->page.lines[i].link_idx >= 0) {
                int li = app->page.lines[i].link_idx;
                if (li < app->page.link_count) {
                    char new_url[MAX_URL_LEN];
                    const char* link_url = app->page.links[li].url;
                    if (link_url[0] == '/' || strncmp(link_url, "http", 4) != 0) {
                        // Relative URL
                        const char* base_end = strstr(app->current_url, "//");
                        if (base_end) {
                            base_end += 2;
                            const char* path_start = strchr(base_end, '/');
                            char base[MAX_URL_LEN];
                            if (path_start) {
                                size_t base_len = path_start - app->current_url;
                                strncpy(base, app->current_url, base_len);
                                base[base_len] = 0;
                            } else {
                                strncpy(base, app->current_url, sizeof(base) - 1);
                            }
                            if (link_url[0] == '/') {
                                snprintf(new_url, sizeof(new_url), "%s%s", base, link_url);
                            } else {
                                snprintf(new_url, sizeof(new_url), "%s/%s", base, link_url);
                            }
                        } else {
                            strncpy(new_url, link_url, sizeof(new_url) - 1);
                        }
                        fetch_page(app, new_url);
                    } else {
                        fetch_page(app, link_url);
                    }
                }
                break;
            }
        }
    }
    if (key & HidNpadButton_B) {
        app->state = STATE_HOME;
        app->selected_idx = 0;
    }
    if (key & HidNpadButton_X) {
        char input[256];
        if (browser_input_url(input, sizeof(input), app->current_url)) {
            if (browser_is_url(input)) {
                browser_normalize_url(input, sizeof(input));
                fetch_page(app, input);
            } else {
                char search_url[MAX_URL_LEN];
                browser_build_search_url(search_url, sizeof(search_url), input, NULL);
                fetch_page(app, search_url);
            }
        }
    }
}

static void handle_bookmarks(AppContext* app, u64 key) {
    if (app->bookmark_count == 0) {
        if (key & HidNpadButton_B) app->state = app->prev_state;
        return;
    }
    if (key & HidNpadButton_Down && app->selected_idx < app->bookmark_count - 1) app->selected_idx++;
    if (key & HidNpadButton_Up && app->selected_idx > 0) app->selected_idx--;
    if (key & HidNpadButton_A) {
        fetch_page(app, app->bookmarks[app->selected_idx].url);
    }
    if (key & HidNpadButton_B) app->state = app->prev_state;
}

static void console_main(void) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\n\n  SwitchBrowser v" APP_VERSION "\n");
    printf("  ========================\n\n");
    printf("  SDL2 init failed. Console mode.\n\n");
    printf("  Press [+] to exit.\n\n");

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 key = padGetButtonsDown(&pad);
        if (key & HidNpadButton_Plus) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
}

int main(int argc, char* argv[]) {
    AppContext app;
    memset(&app, 0, sizeof(app));

    if (!ui_init(&app.ui)) {
        console_main();
        return 0;
    }

    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    bookmarks_load(&app);
    strcpy(app.config.homepage, "https://www.baidu.com");
    strcpy(app.config.search_engine, "https://www.baidu.com/s?wd=");
    app.state = STATE_HOME;
    app.selected_idx = 0;
    app.scroll_offset = 0;

    while (appletMainLoop() && !app.exit_app) {
        u64 key = pad_get_keys(&app.ui.pad);

        if (key & HidNpadButton_Plus) {
            app.exit_app = true;
            break;
        }

        switch (app.state) {
            case STATE_HOME:
                handle_home(&app, key);
                draw_home(&app);
                break;
            case STATE_LOADING:
                draw_loading(&app);
                break;
            case STATE_PAGE_VIEW:
                handle_page_view(&app, key);
                draw_page(&app);
                break;
            case STATE_BOOKMARKS:
                handle_bookmarks(&app, key);
                draw_bookmarks(&app);
                break;
            case STATE_ERROR:
                draw_page(&app);
                if (key & (HidNpadButton_B | HidNpadButton_A)) {
                    app.state = STATE_HOME;
                    app.selected_idx = 0;
                }
                break;
            default:
                app.state = STATE_HOME;
                draw_home(&app);
                break;
        }
    }

    bookmarks_save(&app);
    curl_global_cleanup();
    socketExit();
    ui_exit(&app.ui);
    return 0;
}
