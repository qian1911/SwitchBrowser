#pragma once
#include "types.h"

/// Clear screen and draw header.
void ui_draw_header(const char *subtitle);

/// Draw the main menu.
void ui_draw_main_menu(AppContext *ctx);

/// Draw the bookmarks list.
void ui_draw_bookmarks(AppContext *ctx);

/// Draw the history list.
void ui_draw_history(AppContext *ctx);

/// Draw the settings screen.
void ui_draw_settings(AppContext *ctx);

/// Draw the about screen.
void ui_draw_about(void);

/// Draw a status bar at the bottom of the screen.
void ui_draw_status_bar(const char *hint);

/// Print a centered string at a given Y row.
void ui_print_centered(int row, const char *text);

/// Clear a specific line.
void ui_clear_line(int row);
