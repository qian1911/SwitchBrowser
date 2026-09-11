#include "ui.h"
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static void set_color(UIContext* ctx, u32 c) {
    SDL_SetRenderDrawColor(ctx->renderer, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
}

bool ui_init(UIContext* ctx) {
    memset(ctx, 0, sizeof(UIContext));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    if (TTF_Init() < 0) {
        printf("TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) {
        printf("plInitialize: 0x%x\n", rc);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN, &ctx->window, &ctx->renderer) < 0) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        plExit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    PlFontData font_data;
    rc = plGetSharedFontByType(&font_data, PlSharedFontType_Standard);
    if (R_FAILED(rc)) {
        printf("plGetSharedFont: 0x%x\n", rc);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        plExit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    ctx->font = TTF_OpenFontRW(SDL_RWFromMem(font_data.address, font_data.size), 0, 18);
    ctx->font_small = TTF_OpenFontRW(SDL_RWFromMem(font_data.address, font_data.size), 0, 14);
    ctx->font_large = TTF_OpenFontRW(SDL_RWFromMem(font_data.address, font_data.size), 0, 24);
    if (!ctx->font || !ctx->font_small || !ctx->font_large) {
        printf("TTF_OpenFont: %s\n", TTF_GetError());
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        plExit();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&ctx->pad);

    AppletType at = appletGetAppletType();
    ctx->applet_mode = (at != AppletType_Application);

    set_color(ctx, COL_BG);
    SDL_RenderClear(ctx->renderer);
    ui_draw_text_centered(ctx, "SwitchBrowser v2.0", 0, 300, SCREEN_WIDTH, 28, COL_ACCENT);
    ui_draw_text_centered(ctx, "Loading...", 0, 360, SCREEN_WIDTH, 18, COL_TEXT_DIM);
    SDL_RenderPresent(ctx->renderer);

    return true;
}

void ui_exit(UIContext* ctx) {
    if (ctx->font) TTF_CloseFont(ctx->font);
    if (ctx->font_small) TTF_CloseFont(ctx->font_small);
    if (ctx->font_large) TTF_CloseFont(ctx->font_large);
    plExit();
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
        {x + r, y, w - 2*r, r},
        {x + r, y + h - r, w - 2*r, r},
        {x, y + r, w, h - 2*r},
        {x, y + r, r, h - 2*r},
        {x + w - r, y + r, r, h - 2*r},
    };
    SDL_RenderFillRects(ctx->renderer, rects, 5);
    for (int dy = 0; dy <= r; dy++) {
        int dx = (int)sqrtf((float)(r*r - dy*dy));
        SDL_RenderDrawLines(ctx->renderer, (SDL_Point[]){
            {x + r - dx, y + r - dy}, {x + r + dx, y + r - dy}
        }, 2);
        SDL_RenderDrawLines(ctx->renderer, (SDL_Point[]){
            {x + r - dx, y + h - r + dy}, {x + r + dx, y + h - r + dy}
        }, 2);
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
    SDL_Color c = {(color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF};
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, text, c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ctx->renderer, s);
    if (t) {
        SDL_Rect d = {x, y, s->w, s->h};
        SDL_RenderCopy(ctx->renderer, t, NULL, &d);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
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
    char buf[2048];
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
            char current[1024] = "";
            char* tok = strtok(line, " ");
            while (tok) {
                char test[1280];
                snprintf(test, sizeof(test), "%s %s", current, tok);
                TTF_SizeUTF8(font, test, &tw, &th);
                if (tw > max_w && current[0]) {
                    ui_draw_text(ctx, current, x, yy, size, color);
                    yy += line_h;
                    strncpy(current, tok, sizeof(current) - 1);
                } else {
                    if (current[0]) strncat(current, " ", sizeof(current) - strlen(current) - 1);
                    strncat(current, tok, sizeof(current) - strlen(current) - 1);
                }
                tok = strtok(NULL, " ");
            }
            if (current[0]) { ui_draw_text(ctx, current, x, yy, size, color); yy += line_h; }
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

void ui_present(UIContext* ctx) { SDL_RenderPresent(ctx->renderer); }

bool ui_button(UIContext* ctx, UIRect r, const char* text, bool selected) {
    u32 bg = selected ? COL_ACCENT : COL_CARD;
    if (selected) ui_fill_rounded_rect(ctx, r.x - 2, r.y - 2, r.w + 4, r.h + 4, 12, COL_ACCENT_D);
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 10, bg);
    ui_draw_text_centered(ctx, text, r.x, r.y + (r.h - 18) / 2, r.w, 18, COL_TEXT);
    return selected;
}

void ui_top_bar(UIContext* ctx, const char* title, const char* subtitle) {
    ui_fill_rect(ctx, 0, 0, SCREEN_WIDTH, TOP_BAR_H, COL_CARD);
    ui_fill_rect(ctx, 0, TOP_BAR_H - 3, SCREEN_WIDTH, 3, COL_ACCENT);
    ui_draw_text(ctx, title, PADDING, (TOP_BAR_H - 24) / 2, 24, COL_TEXT);
    if (subtitle && subtitle[0])
        ui_draw_text(ctx, subtitle, PADDING + ui_text_width(ctx, title, 24) + 20, (TOP_BAR_H - 14) / 2 + 5, 14, COL_TEXT_DIM);
}

void ui_bottom_bar(UIContext* ctx, const char* text) {
    int y = SCREEN_HEIGHT - BOT_BAR_H;
    ui_fill_rect(ctx, 0, y, SCREEN_WIDTH, BOT_BAR_H, COL_CARD);
    ui_fill_rect(ctx, 0, y, SCREEN_WIDTH, 1, COL_CARD_HL);
    ui_draw_text(ctx, text, PADDING, y + (BOT_BAR_H - 14) / 2, 14, COL_TEXT_DIM);
}

void ui_input_box(UIContext* ctx, UIRect r, const char* text, bool focused) {
    u32 border = focused ? COL_ACCENT : COL_CARD_HL;
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 12, COL_CARD);
    set_color(ctx, border);
    SDL_Rect outline = {r.x, r.y, r.w, r.h};
    SDL_RenderDrawRect(ctx->renderer, &outline);
    const char* display = (text && text[0]) ? text : "Search or enter URL...";
    u32 tc = (text && text[0]) ? COL_TEXT : COL_TEXT_DIM;
    ui_draw_text(ctx, display, r.x + 20, r.y + (r.h - 18) / 2, 18, tc);
    if (focused) {
        int tw = ui_text_width(ctx, display, 18);
        set_color(ctx, COL_ACCENT);
        SDL_Rect cursor = {r.x + 20 + tw + 2, r.y + (r.h - 18) / 2, 2, 22};
        SDL_RenderFillRect(ctx->renderer, &cursor);
    }
}

bool ui_quick_link_card(UIContext* ctx, UIRect r, const char* name, const char* url, bool selected) {
    u32 bg = selected ? COL_CARD_HL : COL_CARD;
    if (selected) ui_fill_rounded_rect(ctx, r.x - 2, r.y - 2, r.w + 4, r.h + 4, 14, COL_ACCENT_D);
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 12, bg);

    set_color(ctx, COL_ACCENT);
    SDL_Rect icon_bg = {r.x + 12, r.y + 12, 48, 48};
    SDL_RenderFillRect(ctx->renderer, &icon_bg);

    char letter[2] = {name[0] ? name[0] : '?', 0};
    ui_draw_text_centered(ctx, letter, r.x + 12, r.y + 22, 48, 24, COL_TEXT);

    ui_draw_text(ctx, name, r.x + 76, r.y + 14, 20, COL_TEXT);
    ui_draw_text(ctx, url, r.x + 76, r.y + 42, 14, COL_TEXT_DIM);
    return selected;
}

void ui_progress_bar(UIContext* ctx, UIRect r, float progress) {
    ui_fill_rounded_rect(ctx, r.x, r.y, r.w, r.h, 4, COL_CARD);
    int fill_w = (int)(r.w * progress);
    if (fill_w > 0) ui_fill_rounded_rect(ctx, r.x, r.y, fill_w, r.h, 4, COL_ACCENT);
}

u64 pad_get_keys(PadState* pad) {
    padUpdate(pad);
    return padGetButtonsDown(pad);
}
