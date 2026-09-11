#ifndef UI_H
#define UI_H

#include "types.h"

#define COL_BG       0x1A1A2E
#define COL_CARD     0x2D2D44
#define COL_CARD_HL  0x3D3D5C
#define COL_ACCENT   0x4FC3F7
#define COL_ACCENT_D 0x29B6F6
#define COL_TEXT     0xFFFFFF
#define COL_TEXT_DIM 0xB0B0C0
#define COL_SUCCESS  0x66BB6A
#define COL_ERROR    0xEF5350
#define COL_WARNING  0xFFCA28
#define COL_LINK     0x64B5F6

#define TOP_BAR_H    72
#define BOT_BAR_H    56
#define PADDING      24

typedef struct { int x, y, w, h; } UIRect;

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

bool ui_button(UIContext* ctx, UIRect r, const char* text, bool selected);
void ui_top_bar(UIContext* ctx, const char* title, const char* subtitle);
void ui_bottom_bar(UIContext* ctx, const char* text);
void ui_input_box(UIContext* ctx, UIRect r, const char* text, bool focused);
bool ui_quick_link_card(UIContext* ctx, UIRect r, const char* name, const char* url, bool selected);
void ui_progress_bar(UIContext* ctx, UIRect r, float progress);

u64 pad_get_keys(PadState* pad);

#endif
