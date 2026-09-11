#include <string.h>
#include <stdio.h>
#include <switch.h>

#include "ui.h"
#include "types.h"

#define CONSOLE_WIDTH 80

void ui_print_centered(int row, const char *text) {
    int len = (int)strlen(text);
    int x = (CONSOLE_WIDTH - len) / 2;
    if (x < 0) x = 0;
    printf("\x1b[%d;%dH%s", row, x, text);
}

void ui_clear_line(int row) {
    printf("\x1b[%d;1H%*s", row, CONSOLE_WIDTH, "");
}

void ui_draw_header(const char *subtitle) {
    consoleClear();
    printf("\n");
    ui_print_centered(1, "========================================");
    ui_print_centered(2, "        SwitchBrowser v" APP_VERSION "        ");
    ui_print_centered(3, "========================================");
    if (subtitle) {
        ui_print_centered(4, subtitle);
    }
    printf("\n");
}

void ui_draw_main_menu(AppContext *ctx) {
    ui_draw_header("Nintendo Switch Web Browser");

    printf("\n");
    printf("  [A] Enter URL / Search\n");
    printf("  [B] Bookmarks\n");
    printf("  [Y] History\n");
    printf("  [X] Settings\n");
    printf("\n");
    printf("  --- Quick Access ---\n\n");

    Bookmark defaults[12];
    int dcount = 0;
    extern void bookmarks_get_defaults(Bookmark*, int*, int);
    bookmarks_get_defaults(defaults, &dcount, 12);

    int row = 13;
    int col = 0;
    for (int i = 0; i < dcount && i < 8; i++) {
        printf("\x1b[%d;%dH [%d] %-20s", row, col * 26 + 2, i + 1, defaults[i].title);
        col++;
        if (col >= 3) {
            col = 0;
            row++;
        }
    }

    printf("\n\n");
    printf("  --- Current Session ---\n");
    if (ctx->last_url[0]) {
        printf("  Last visited: %.70s\n", ctx->last_url);
    } else {
        printf("  Last visited: (none)\n");
    }

    ui_draw_status_bar("[+] Exit  [A] URL  [B] Bookmarks  [Y] History  [X] Settings");
}

void ui_draw_bookmarks(AppContext *ctx) {
    ui_draw_header("Bookmarks");

    if (ctx->bookmark_count == 0) {
        printf("\n  No bookmarks saved.\n\n");
        printf("  Visit a page and add it to bookmarks.\n");
    } else {
        printf("  %-3s  %-25s  %s\n", "#", "Title", "URL");
        printf("  %-3s  %-25s  %s\n", "---", "-------------------------", "----------");

        int start = ctx->scroll_offset;
        int max_display = 18;

        for (int i = start; i < ctx->bookmark_count && i < start + max_display; i++) {
            const char *cursor = (i == ctx->selected_idx) ? ">>" : "  ";
            printf("%s[%2d] %-25.25s  %.45s\n", cursor, i + 1,
                   ctx->bookmarks[i].title, ctx->bookmarks[i].url);
        }
    }

    ui_draw_status_bar("[A] Open  [X] Add Current  [Y] Delete  [B]/[+] Back");
}

void ui_draw_history(AppContext *ctx) {
    ui_draw_header("Browsing History");

    if (ctx->history_count == 0) {
        printf("\n  No browsing history.\n\n");
        printf("  Start browsing to build history.\n");
    } else {
        printf("  %-3s  %-25s  %s\n", "#", "Title", "URL");
        printf("  %-3s  %-25s  %s\n", "---", "-------------------------", "----------");

        int start = ctx->scroll_offset;
        int max_display = 18;

        for (int i = start; i < ctx->history_count && i < start + max_display; i++) {
            const char *cursor = (i == ctx->selected_idx) ? ">>" : "  ";
            printf("%s[%2d] %-25.25s  %.45s\n", cursor, i + 1,
                   ctx->history[i].title, ctx->history[i].url);
        }
    }

    ui_draw_status_bar("[A] Open  [X] Clear All  [B]/[+] Back");
}

void ui_draw_settings(AppContext *ctx) {
    ui_draw_header("Settings");

    printf("\n");
    printf("  Homepage:    %s\n", ctx->config.homepage);
    printf("  Search engine: %s\n", ctx->config.search_engine);
    printf("\n");
    printf("  --- Web Engine Options ---\n\n");
    printf("  [1] JavaScript Extensions: %s\n", ctx->config.enable_js ? "ON" : "OFF");
    printf("  [2] Touch on Content:      %s\n", ctx->config.enable_touch ? "ON" : "OFF");
    printf("  [3] Pointer (Stick Mouse): %s\n", ctx->config.enable_pointer ? "ON" : "OFF");
    printf("  [4] Page Cache:            %s\n", ctx->config.enable_cache ? "ON" : "OFF");
    printf("  [5] Web Audio:             %s\n", ctx->config.enable_audio ? "ON" : "OFF");
    printf("\n");
    printf("  [R] Reset to Defaults\n");

    ui_draw_status_bar("[1-5] Toggle  [A] Edit Homepage  [B]/[+] Back");
}

void ui_draw_about(void) {
    ui_draw_header("About");

    printf("\n\n");
    ui_print_centered(8, APP_TITLE " v" APP_VERSION);
    printf("\n\n");
    ui_print_centered(11, "A web browser for Nintendo Switch");
    ui_print_centered(12, "Powered by Switch WebKit Applet");
    printf("\n\n");
    ui_print_centered(15, "Uses libnx web applet API");
    ui_print_centered(16, "Full HTML5 / CSS / JavaScript support");
    printf("\n\n");
    ui_print_centered(19, "Built with devkitPro + libnx");
    printf("\n\n");
    ui_print_centered(22, "[B] / [+] Back");
}

void ui_draw_status_bar(const char *hint) {
    printf("\n");
    printf("\x1b[30;1H\033[44m%-*s\033[0m", CONSOLE_WIDTH, "");
    printf("\x1b[30;1H\033[44m%s\033[0m", hint ? hint : "");
}
