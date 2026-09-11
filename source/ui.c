#include "ui.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static u32 rgb(u8 r, u8 g, u8 b, u8 a) {
    return ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | b;
}

static void set_color(UIContext* ctx, u32 c) {
    SDL_SetRenderDrawColor(ctx->renderer, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
}

bool ui_init(UIContext* ctx) {
    memset(ctx, 0, sizeof(UIContext));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) return false;
    if (TTF_Init() < 0) return false;

    romfsInit();
    plInitialize(PlServiceType_User);

    if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN, &ctx->window, &ctx->renderer) < 0)
        return false;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    // Load Switch shared font
    PlFontData font_data;
    Result rc = plGetSharedFontByType(&font_data, PlSharedFontType_Standard);
    if (R_FAILED(rc)) return false;

    SDL_RWops* rw = SDL_RWFromMem(font_data.address, font_data.size);
    ctx->font = TTF_OpenFontRW(rw, 0, 18);
    ctx->font_small = TTF_OpenFontRW(SDL_RWFromMem(font_data.address, font_data.size), 0, 14);
    ctx->font_large = TTF_OpenFontRW(SDL_RWFromMem(font_data.address, font_data.size), 0, 24);

    if (!ctx->font || !ctx->font_small || !ctx->font_large) return false;

    // Joystick
    SDL_JoystickEventState(SDL_ENABLE);
    ctx->joystick = SDL_JoystickOpen(0);

    // Pad
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&ctx->pad);

    // Check if running in applet mode (from album/hbmenu)
    AppletType at = appletGetAppletType();
    ctx->applet_mode = (at == AppletType_LibraryApplet ||
                        at == AppletType_LibraryAppletPhotoViewer ||
                        at != AppletType_Application);

    ctx->needs_redraw = true;
    return true;
}

void ui_exit(UIContext* ctx) {
    if (ctx->font) TTF_CloseFont(ctx->font);
    if (ctx->font_small) TTF_CloseFont(ctx->font_small);
    if (ctx->font_large) TTF_CloseFont(ctx->font_large);
    if (ctx->joystick) SDL_JoystickClose(ctx->joystick);
    plExit();
    romfsExit();
    TTF_Quit();
    SDL_Quit();
}

void ui_clear(UIContext* ctx, u32 color) {
    set_color(ctx, color);
    SDL_RenderClear(ctx->renderer);
}

void ui_fill_rect(UIContext* ctx, int x, int y, int w, int h, u32 color) {
    set_color(ctx, color);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(ctx->renderer, &r);
}

void ui_fill_rounded_rect(UIContext* ctx, int x, int y, int w, int h, int r, u32 color) {
    set_color(ctx, color);
    SDL_Rect rects[] = {
        {x + r, y, w - 2*r, r},         // top
        {x + r, y + h - r, w - 2*r, r},  // bottom
        {x, y + r, w, h - 2*r},          // middle
        {x, y + r, r, h - 2*r},          // left
        {x + w - r, y + r, r, h - 2*r},  // right
    };
    SDL_RenderFillRects(ctx->renderer, rects, 5);

    // Corners
    for (int dy = 0; dy <= r; dy++) {
        int dx = (int)sqrtf((float)(r*r - dy*dy));
        SDL_Rect top_l = {x + r - dx, y + r - dy, dx + dx, 1};
        SDL_Rect bot_l = {x + r - dx, y + h - r + dy - 1, dx + dx, 1};
        SDL_RenderFillRects(ctx->renderer, (SDL_Rect[]){top_l, bot_l}, 2);
    }
}

static TTF_Font* get_font(UIContext* ctx, int size) {
    if (size <= 14) return ctx->font_small;
    if (size >= 24) return ctx->font_large;
    return ctx->font;
}

void ui_draw_text(UIContext* ctx, const char* text, int x, int y, int size, u32 color) {
    if (!text || !text[0]) return;
    TTF_Font* font = get_font(ctx, size);
    if (!font) return;

    SDL_Color c = {(color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF};
    if (c.a == 0) c.a = 0xFF;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, c);
    if (!surface) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surface);
    if (tex) {
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surface);
}

void ui_draw_text_centered(UIContext* ctx, const char* text, int x, int y, int w, int size, u32 color) {
    TTF_Font* font = get_font(ctx, size);
    if (!font || !text) return;

    int tw, th;
    TTF_SizeUTF8(font, text, &tw, &th);
    ui_draw_text(ctx, text, x + (w - tw) / 2, y, size, color);
}

void ui_draw_text_wrapped(UIContext* ctx, const char* text, int x, int y, int max_w, int size, u32 color) {
    TTF_Font* font = get_font(ctx, size);
    if (!font || !text) return;

    int line_h = TTF_FontHeight(font) + 2;
    char buf[512];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char* line = strtok(buf, "\n");
    int yy = y;
    while (line) {
        int tw, th;
        TTF_SizeUTF8(font, line, &tw, &th);
        if (tw <= max_w) {
            ui_draw_text(ctx, line, x, yy, size, color);
            yy += line_h;
        } else {
            // Word wrap
            char word[256];
            char current[512] = "";
            char* tok = strtok(line, " ");
            while (tok) {
                char test[768];
                snprintf(test, sizeof(test), "%s %s", current, tok);
                TTF_SizeUTF8(font, test, &tw, &th);
                if (tw > max_w && current[0]) {
                    ui_draw_text(ctx, current, x, yy, size, color);
                    yy += line_h;
                    strncpy(current, tok, sizeof(current) - 1);
                } else {
                    if (current[0]) {
                        strncat(current, " ", sizeof(current) - strlen(current) - 1);
                    }
                    strncat(current, tok, sizeof(current) - strlen(current) - 1);
                }
                tok = strtok(NULL, " ");
            }
            if (current[0]) {
                ui_draw_text(ctx, current, x, yy, size, color);
                yy += line_h;
            }
        }
        line = strtok(NULL, "\n");
    }
}

int ui_text_width(UIContext* ctx, const char* text, int size) {
    TTF_Font* font = get_font(ctx, size);
    if (!font || !text) return 0;
    int w, h;
    TTF_SizeUTF8(font, text, &w, &h);
    return w;
}

void ui_present(UIContext* ctx) {
    SDL_RenderPresent(ctx->renderer);
}

// --- Components ---

bool ui_button(UIContext* ctx, UIRect r, const char* text, bool selected) {
    u32 bg = selected ? COL_ACCENT : COL_CARD;
    if (selected) {
        ui_fill_rounded_rect(ctx, r.x - 2, r.y - 2, r.w + 4, r.h + 4, 12, COL_ACCENT_D);
    }
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 10, bg);
    ui_draw_text_centered(ctx, text, r.x, r.y + (r.h - 18) / 2, r.w, 18, COL_TEXT);
    return selected;
}

bool ui_card(UIContext* ctx, UIRect r, const char* title, const char* subtitle, bool selected) {
    u32 bg = selected ? COL_CARD_HL : COL_CARD;
    if (selected) {
        ui_fill_rounded_rect(ctx, r.x - 2, r.y - 2, r.w + 4, r.h + 4, 14, COL_ACCENT_D);
    }
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 12, bg);

    // Icon circle
    int icon_r = 20;
    int cx = r.x + 28 + icon_r;
    int cy = r.y + r.h / 2;
    set_color(ctx, COL_ACCENT);
    SDL_Rect icon_bg = {cx - icon_r, cy - icon_r, icon_r * 2, icon_r * 2};
    SDL_RenderFillRect(ctx->renderer, &icon_bg);

    // Title and subtitle
    ui_draw_text(ctx, title, r.x + 70, r.y + 14, 18, COL_TEXT);
    if (subtitle && subtitle[0]) {
        ui_draw_text(ctx, subtitle, r.x + 70, r.y + 40, 14, COL_TEXT_DIM);
    }
    return selected;
}

void ui_input_box(UIContext* ctx, UIRect r, const char* text, bool focused) {
    u32 border = focused ? COL_ACCENT : COL_CARD_HL;
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 10, COL_CARD);
    // Border
    set_color(ctx, border);
    SDL_RenderDrawRect(ctx->renderer, &(SDL_Rect){r.x, r.y, r.w, r.h});

    if (text && text[0]) {
        ui_draw_text(ctx, text, r.x + 16, r.y + (r.h - 18) / 2, 18, COL_TEXT);
    } else {
        ui_draw_text(ctx, "Search or type URL", r.x + 16, r.y + (r.h - 14) / 2, 14, COL_TEXT_DIM);
    }
}

void ui_progress_bar(UIContext* ctx, UIRect r, float progress) {
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 4, COL_CARD);
    int fill_w = (int)(r.w * progress);
    if (fill_w > 0) {
        ui_fill_rounded_rect(ctx, r.x, r.y, fill_w, r.h, 4, COL_ACCENT);
    }
}

void ui_top_bar(UIContext* ctx, const char* title) {
    ui_fill_rect(ctx, 0, 0, SCREEN_WIDTH, TOP_BAR_H, COL_BG);
    ui_draw_text(ctx, title, PADDING, (TOP_BAR_H - 24) / 2, 24, COL_TEXT);
    // Accent line
    ui_fill_rect(ctx, 0, TOP_BAR_H - 2, SCREEN_WIDTH, 2, COL_ACCENT);
}

void ui_bottom_bar(UIContext* ctx, int selected_tab) {
    int y = SCREEN_HEIGHT - BOT_BAR_H;
    ui_fill_rect(ctx, 0, y, SCREEN_WIDTH, BOT_BAR_H, COL_CARD);
    ui_fill_rect(ctx, 0, y, SCREEN_WIDTH, 1, COL_CARD_HL);

    const char* labels[] = {"Home", "Bookmarks", "History", "Settings"};
    int tab_w = SCREEN_WIDTH / 4;
    for (int i = 0; i < 4; i++) {
        int tx = i * tab_w + tab_w / 2;
        u32 color = (i == selected_tab) ? COL_ACCENT : COL_TEXT_DIM;
        ui_draw_text_centered(ctx, labels[i], i * tab_w, y + (BOT_BAR_H - 18) / 2, tab_w, 18, color);
        if (i == selected_tab) {
            int tw = ui_text_width(ctx, labels[i], 18);
            ui_fill_rect(ctx, tx - tw / 2, y + BOT_BAR_H - 4, tw, 3, COL_ACCENT);
        }
    }
}

void ui_draw_icon_globe(UIContext* ctx, int cx, int cy, int r, u32 color) {
    set_color(ctx, color);
    // Circle
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)sqrtf((float)(r*r - dy*dy));
        SDL_RenderDrawLine(ctx->renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
    // Meridians
    set_color(ctx, COL_BG);
    SDL_RenderDrawLine(ctx->renderer, cx, cy - r, cx, cy + r);
    int r2 = r * 3 / 4;
    for (int dy = -r2; dy <= r2; dy += 4) {
        int dx = (int)sqrtf((float)(r2*r2 - dy*dy));
        SDL_RenderDrawLine(ctx->renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

bool rect_contains(UIRect r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

u64 pad_get_keys(PadState* pad) {
    padUpdate(pad);
    return padGetButtonsDown(pad);
}
