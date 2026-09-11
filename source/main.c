#include <string.h>
#include <stdio.h>
#include <switch.h>

#include "types.h"
#include "browser.h"
#include "bookmarks.h"
#include "history.h"
#include "ui.h"

// Key mappings for libnx 4.12+
#define KEY_A       HidNpadButton_A
#define KEY_B       HidNpadButton_B
#define KEY_X       HidNpadButton_X
#define KEY_Y       HidNpadButton_Y
#define KEY_UP      HidNpadButton_Up
#define KEY_DOWN    HidNpadButton_Down
#define KEY_LEFT    HidNpadButton_Left
#define KEY_RIGHT   HidNpadButton_Right
#define KEY_PLUS    HidNpadButton_Plus
#define KEY_MINUS   HidNpadButton_Minus
#define KEY_R       HidNpadButton_R
#define KEY_L       HidNpadButton_L
#define KEY_DUP     HidNpadButton_Up
#define KEY_DDOWN   HidNpadButton_Down
#define KEY_DLEFT   HidNpadButton_Left
#define KEY_DRIGHT  HidNpadButton_Right

static AppContext g_ctx;
static bool g_exit = false;

static void load_config(BrowserConfig *cfg) {
    memset(cfg, 0, sizeof(BrowserConfig));
    strncpy(cfg->homepage, "https://www.google.com", MAX_URL_LEN - 1);
    strncpy(cfg->search_engine, "https://www.google.com/search?q=", MAX_URL_LEN - 1);
    cfg->enable_js = true;
    cfg->enable_touch = true;
    cfg->enable_pointer = true;
    cfg->enable_cache = true;
    cfg->enable_audio = true;
}

static void init_context(AppContext *ctx) {
    memset(ctx, 0, sizeof(AppContext));
    ctx->state = STATE_MAIN_MENU;
    ctx->prev_state = STATE_MAIN_MENU;
    ctx->selected_idx = 0;
    ctx->scroll_offset = 0;
    ctx->needs_redraw = true;
    load_config(&ctx->config);
    bookmarks_load(ctx->bookmarks, &ctx->bookmark_count, MAX_BOOKMARKS);
    history_load(ctx->history, &ctx->history_count, MAX_HISTORY);
}

static void handle_main_menu(u64 key, AppContext *ctx) {
    if (key & KEY_A) {
        ctx->state = STATE_URL_INPUT;
        ctx->needs_redraw = true;
    } else if (key & KEY_B) {
        ctx->state = STATE_BOOKMARKS;
        ctx->selected_idx = 0;
        ctx->scroll_offset = 0;
        ctx->needs_redraw = true;
    } else if (key & KEY_Y) {
        ctx->state = STATE_HISTORY;
        ctx->selected_idx = 0;
        ctx->scroll_offset = 0;
        ctx->needs_redraw = true;
    } else if (key & KEY_X) {
        ctx->state = STATE_SETTINGS;
        ctx->needs_redraw = true;
    } else if (key & KEY_PLUS) {
        g_exit = true;
    }
}

static void handle_url_input(AppContext *ctx) {
    char input[MAX_URL_LEN];

    if (browser_input_url(input, sizeof(input), "https://")) {
        browser_normalize_url(input, sizeof(input));

        if (strncmp(input, "http://", 7) == 0 || strncmp(input, "https://", 8) == 0) {
            strncpy(ctx->current_url, input, MAX_URL_LEN - 1);
            ctx->current_url[MAX_URL_LEN - 1] = '\0';
        } else {
            browser_build_search_url(ctx->current_url, MAX_URL_LEN,
                                     input, ctx->config.search_engine);
        }

        history_add(ctx->history, &ctx->history_count, MAX_HISTORY,
                    ctx->current_url, "");
        ctx->state = STATE_BROWSING;
    } else {
        ctx->state = STATE_MAIN_MENU;
    }
    ctx->needs_redraw = true;
}

static void handle_browsing(AppContext *ctx) {
    char last_url[MAX_URL_LEN] = {0};

    bool success = browser_navigate(ctx->current_url, last_url, sizeof(last_url));

    if (success && last_url[0]) {
        strncpy(ctx->last_url, last_url, MAX_URL_LEN - 1);
        ctx->last_url[MAX_URL_LEN - 1] = '\0';
        history_add(ctx->history, &ctx->history_count, MAX_HISTORY, last_url, "");
    }

    ctx->state = STATE_MAIN_MENU;
    ctx->needs_redraw = true;
}

static void handle_bookmarks(u64 key, AppContext *ctx) {
    if (ctx->bookmark_count == 0) {
        if (key & (KEY_B | KEY_PLUS)) {
            ctx->state = STATE_MAIN_MENU;
            ctx->needs_redraw = true;
        }
        return;
    }

    if (key & KEY_DUP) {
        if (ctx->selected_idx > 0) {
            ctx->selected_idx--;
            if (ctx->selected_idx < ctx->scroll_offset)
                ctx->scroll_offset = ctx->selected_idx;
            ctx->needs_redraw = true;
        }
    } else if (key & KEY_DDOWN) {
        if (ctx->selected_idx < ctx->bookmark_count - 1) {
            ctx->selected_idx++;
            if (ctx->selected_idx >= ctx->scroll_offset + 18)
                ctx->scroll_offset = ctx->selected_idx - 17;
            ctx->needs_redraw = true;
        }
    }

    if (key & KEY_A) {
        strncpy(ctx->current_url, ctx->bookmarks[ctx->selected_idx].url, MAX_URL_LEN - 1);
        ctx->current_url[MAX_URL_LEN - 1] = '\0';
        history_add(ctx->history, &ctx->history_count, MAX_HISTORY,
                    ctx->current_url, ctx->bookmarks[ctx->selected_idx].title);
        ctx->state = STATE_BROWSING;
        ctx->needs_redraw = true;
    } else if (key & KEY_Y) {
        bookmarks_remove(ctx->bookmarks, &ctx->bookmark_count, ctx->selected_idx);
        if (ctx->selected_idx >= ctx->bookmark_count && ctx->selected_idx > 0)
            ctx->selected_idx--;
        ctx->needs_redraw = true;
    } else if (key & KEY_X) {
        if (ctx->last_url[0]) {
            bookmarks_add(ctx->bookmarks, &ctx->bookmark_count, MAX_BOOKMARKS,
                          ctx->last_url, "");
            ctx->needs_redraw = true;
        }
    } else if (key & (KEY_B | KEY_PLUS)) {
        ctx->state = STATE_MAIN_MENU;
        ctx->needs_redraw = true;
    }
}

static void handle_history(u64 key, AppContext *ctx) {
    if (ctx->history_count == 0) {
        if (key & (KEY_B | KEY_PLUS)) {
            ctx->state = STATE_MAIN_MENU;
            ctx->needs_redraw = true;
        }
        return;
    }

    if (key & KEY_DUP) {
        if (ctx->selected_idx > 0) {
            ctx->selected_idx--;
            if (ctx->selected_idx < ctx->scroll_offset)
                ctx->scroll_offset = ctx->selected_idx;
            ctx->needs_redraw = true;
        }
    } else if (key & KEY_DDOWN) {
        if (ctx->selected_idx < ctx->history_count - 1) {
            ctx->selected_idx++;
            if (ctx->selected_idx >= ctx->scroll_offset + 18)
                ctx->scroll_offset = ctx->selected_idx - 17;
            ctx->needs_redraw = true;
        }
    }

    if (key & KEY_A) {
        strncpy(ctx->current_url, ctx->history[ctx->selected_idx].url, MAX_URL_LEN - 1);
        ctx->current_url[MAX_URL_LEN - 1] = '\0';
        ctx->state = STATE_BROWSING;
        ctx->needs_redraw = true;
    } else if (key & KEY_X) {
        history_clear(ctx->history, &ctx->history_count);
        ctx->needs_redraw = true;
    } else if (key & (KEY_B | KEY_PLUS)) {
        ctx->state = STATE_MAIN_MENU;
        ctx->needs_redraw = true;
    }
}

static void handle_settings(u64 key, AppContext *ctx) {
    if (key & KEY_A) {
        char input[MAX_URL_LEN];
        if (browser_input_text(input, sizeof(input), "Edit Homepage", ctx->config.homepage)) {
            strncpy(ctx->config.homepage, input, MAX_URL_LEN - 1);
            ctx->config.homepage[MAX_URL_LEN - 1] = '\0';
            ctx->needs_redraw = true;
        }
    }

    if (key & (KEY_B | KEY_PLUS)) {
        ctx->state = STATE_MAIN_MENU;
        ctx->needs_redraw = true;
    }
}

int main(int argc, char *argv[]) {
    // Init console FIRST — before anything else
    consoleInit(NULL);

    // Init pad
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    // Init app context
    init_context(&g_ctx);

    // Print initial message
    printf("\nSwitchBrowser v" APP_VERSION "\n");
    printf("Loading...\n");
    consoleUpdate(NULL);

    // Small delay to let console render
    svcSleepThread(200000000);  // 200ms

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 key = padGetButtonsDown(&pad);

        if (g_exit) break;

        // Handle states that require console exit
        if (g_ctx.state == STATE_URL_INPUT) {
            consoleExit(NULL);
            handle_url_input(&g_ctx);
            consoleInit(NULL);
            continue;
        }

        if (g_ctx.state == STATE_BROWSING) {
            consoleExit(NULL);
            handle_browsing(&g_ctx);
            consoleInit(NULL);
            continue;
        }

        // Handle console-based states
        switch (g_ctx.state) {
            case STATE_MAIN_MENU:
                handle_main_menu(key, &g_ctx);
                break;
            case STATE_BOOKMARKS:
                handle_bookmarks(key, &g_ctx);
                break;
            case STATE_HISTORY:
                handle_history(key, &g_ctx);
                break;
            case STATE_SETTINGS:
                handle_settings(key, &g_ctx);
                break;
            case STATE_ABOUT:
                if (key & (KEY_B | KEY_PLUS)) {
                    g_ctx.state = STATE_MAIN_MENU;
                    g_ctx.needs_redraw = true;
                }
                break;
            default:
                break;
        }

        // Draw current screen
        if (g_ctx.needs_redraw) {
            switch (g_ctx.state) {
                case STATE_MAIN_MENU:
                    ui_draw_main_menu(&g_ctx);
                    break;
                case STATE_BOOKMARKS:
                    ui_draw_bookmarks(&g_ctx);
                    break;
                case STATE_HISTORY:
                    ui_draw_history(&g_ctx);
                    break;
                case STATE_SETTINGS:
                    ui_draw_settings(&g_ctx);
                    break;
                case STATE_ABOUT:
                    ui_draw_about();
                    break;
                default:
                    break;
            }
            g_ctx.needs_redraw = false;
        }

        consoleUpdate(NULL);
        svcSleepThread(1000000);  // 1ms
    }

    consoleExit(NULL);
    return 0;
}
