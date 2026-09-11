#pragma once
#include <switch.h>

#define APP_TITLE    "SwitchBrowser"
#define APP_VERSION  "1.0.0"
#define MAX_URL_LEN   2048
#define MAX_TITLE_LEN 256
#define MAX_BOOKMARKS 64
#define MAX_HISTORY    100
#define BOOKMARKS_FILE "sdmc:/switch/SwitchBrowser/bookmarks.txt"
#define HISTORY_FILE   "sdmc:/switch/SwitchBrowser/history.txt"
#define CONFIG_FILE    "sdmc:/switch/SwitchBrowser/config.txt"

typedef enum {
    STATE_MAIN_MENU,
    STATE_URL_INPUT,
    STATE_BOOKMARKS,
    STATE_HISTORY,
    STATE_SETTINGS,
    STATE_BROWSING,
    STATE_ABOUT
} AppState;

typedef struct {
    char url[MAX_URL_LEN];
    char title[MAX_TITLE_LEN];
} Bookmark;

typedef struct {
    char url[MAX_URL_LEN];
    char title[MAX_TITLE_LEN];
    u64 timestamp;
} HistoryEntry;

typedef struct {
    char homepage[MAX_URL_LEN];
    char search_engine[MAX_URL_LEN];
    bool enable_js;
    bool enable_touch;
    bool enable_pointer;
    bool enable_cache;
    bool enable_audio;
} BrowserConfig;

typedef struct {
    AppState state;
    AppState prev_state;
    char current_url[MAX_URL_LEN];
    char last_url[MAX_URL_LEN];
    Bookmark bookmarks[MAX_BOOKMARKS];
    int bookmark_count;
    HistoryEntry history[MAX_HISTORY];
    int history_count;
    BrowserConfig config;
    int selected_idx;
    int scroll_offset;
    bool needs_redraw;
} AppContext;
