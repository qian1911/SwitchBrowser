#ifndef UI_H
#define UI_H

#include "types.h"

// Colors (RGB)
#define COL_BG       0x121212
#define COL_CARD     0x1E1E1E
#define COL_CARD_HL  0x2D2D2D
#define COL_ACCENT   0x2196F3
#define COL_ACCENT_D 0x1976D2
#define COL_TEXT      0xFFFFFF
#define COL_TEXT_DIM  0x9E9E9E
#define COL_SUCCESS   0x4CAF50
#define COL_ERROR     0xF44336
#define COL_WHITE     0xFFFFFF
#define COL_TRANSP    0x00000001

// Layout
#define TOP_BAR_H    72
#define BOT_BAR_H    64
#define CARD_H       80
#define CARD_MARGIN  12
#define PADDING      24

bool ui_init(UIContext* ctx);
void ui_exit(UIContext* ctx);
void ui_clear(UIContext* ctx, u32 color);
void ui_fill_rect(UIContext* ctx, int x, int y, int w, int h, u32 color);
void ui_fill_rounded_rect(UIContext* ctx, int x, int y, int w, int h, int r, u32 color);
void ui_draw_text(UIContext* ctx, const char* text, int x, int y, int size, u32 color);
void ui_draw_text_centered(UIContext* ctx, const char* text, int x, int y, int w, int size, u32 color);
void ui_draw_text_wrapped(UIContext* ctx, const char* text, int x, int y, int max_w, int size, u32 color);
int ui_text_width(UIContext* ctx, const char* text, int size);
void ui_present(UIContext* ctx);

// UI components
typedef struct { int x, y, w, h; } UIRect;

bool ui_button(UIContext* ctx, UIRect r, const char* text, bool selected);
bool ui_card(UIContext* ctx, UIRect r, const char* title, const char* subtitle, bool selected);
void ui_input_box(UIContext* ctx, UIRect r, const char* text, bool focused);
void ui_progress_bar(UIContext* ctx, UIRect r, float progress);
void ui_top_bar(UIContext* ctx, const char* title);
void ui_bottom_bar(UIContext* ctx, int selected_tab);
void ui_draw_icon_globe(UIContext* ctx, int cx, int cy, int r, u32 color);

// Input helpers
bool rect_contains(UIRect r, int x, int y);
u64 pad_get_keys(PadState* pad);

#endif
