#ifndef TYPES_H
#define TYPES_H

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define MAX_URL_LEN     512
#define MAX_BOOKMARKS  50
#define MAX_HISTORY     100
#define MAX_TEXT_LEN   256

typedef enum {
    STATE_HOME = 0,
    STATE_URL_INPUT,
    STATE_BOOKMARKS,
    STATE_HISTORY,
    STATE_SETTINGS,
    STATE_BROWSING,
    STATE_ABOUT
} AppState;

typedef struct {
    char title[MAX_TEXT_LEN];
    char url[MAX_URL_LEN];
} Bookmark;

typedef struct {
    char url[MAX_URL_LEN];
    u64 timestamp;
} HistoryEntry;

typedef struct {
    char homepage[MAX_URL_LEN];
    char search_engine[MAX_URL_LEN];
    bool enable_js;
    bool enable_touch;
    bool enable_pointer;
} BrowserConfig;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* font_small;
    TTF_Font* font_large;
    SDL_Joystick* joystick;
    PadState pad;
    bool applet_mode;
    bool needs_redraw;
} UIContext;

typedef struct {
    AppState state;
    AppState prev_state;
    UIContext ui;
    BrowserConfig config;
    Bookmark bookmarks[MAX_BOOKMARKS];
    int bookmark_count;
    HistoryEntry history[MAX_HISTORY];
    int history_count;
    char current_url[MAX_URL_LEN];
    char last_url[MAX_URL_LEN];
    int selected_idx;
    int scroll_offset;
    bool exit_app;
} AppContext;

#endif
