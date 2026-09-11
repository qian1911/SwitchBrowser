#ifndef TYPES_H
#define TYPES_H

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define MAX_URL_LEN     512
#define MAX_BOOKMARKS  50
#define MAX_TEXT_LEN   256
#define MAX_PAGE_LINES 200
#define MAX_LINE_LEN   120
#define MAX_LINKS      50

typedef enum {
    STATE_HOME = 0,
    STATE_URL_INPUT,
    STATE_BOOKMARKS,
    STATE_SETTINGS,
    STATE_LOADING,
    STATE_PAGE_VIEW,
    STATE_ERROR
} AppState;

typedef struct {
    char title[MAX_TEXT_LEN];
    char url[MAX_URL_LEN];
} Bookmark;

typedef struct {
    char text[MAX_LINE_LEN];
    int link_idx;
} PageLine;

typedef struct {
    char text[MAX_LINE_LEN];
    char url[MAX_URL_LEN];
} PageLink;

typedef struct {
    char title[MAX_TEXT_LEN];
    PageLine lines[MAX_PAGE_LINES];
    int line_count;
    PageLink links[MAX_LINKS];
    int link_count;
    int http_status;
    char error_msg[MAX_TEXT_LEN];
} PageContent;

typedef struct {
    char homepage[MAX_URL_LEN];
    char search_engine[MAX_URL_LEN];
} BrowserConfig;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* font_small;
    TTF_Font* font_large;
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
    char current_url[MAX_URL_LEN];
    char current_title[MAX_TEXT_LEN];
    PageContent page;
    int selected_link;
    int selected_idx;
    int scroll_offset;
    bool exit_app;
} AppContext;

#endif
