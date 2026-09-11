#include "bookmarks.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>

#define BOOKMARKS_PATH "sdmc:/switch/SwitchBrowser/bookmarks.txt"
#define BOOKMARKS_DIR  "sdmc:/switch/SwitchBrowser"

static const char* default_bookmarks[] = {
    "Google|https://www.google.com",
    "YouTube|https://www.youtube.com",
    "Wikipedia|https://www.wikipedia.org",
    "Bilibili|https://www.bilibili.com",
    "GitHub|https://github.com",
    "Reddit|https://www.reddit.com",
    "Twitter|https://twitter.com",
    "Amazon|https://www.amazon.com",
    "Bing|https://www.bing.com",
    "DuckDuckGo|https://duckduckgo.com",
};

void bookmarks_load(Bookmark* bookmarks, int* count, int max) {
    *count = 0;
    mkdir(BOOKMARKS_DIR, 0777);

    FILE* f = fopen(BOOKMARKS_PATH, "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f) && *count < max) {
            int len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[--len] = 0;
            if (len > 0 && line[len-1] == '\r') line[--len] = 0;
            if (!len) continue;

            char* sep = strchr(line, '|');
            if (sep) {
                *sep = 0;
                strncpy(bookmarks[*count].title, line, MAX_TEXT_LEN - 1);
                strncpy(bookmarks[*count].url, sep + 1, MAX_URL_LEN - 1);
                bookmarks[*count].title[MAX_TEXT_LEN - 1] = 0;
                bookmarks[*count].url[MAX_URL_LEN - 1] = 0;
                (*count)++;
            }
        }
        fclose(f);
    }

    // Load defaults if no bookmarks
    if (*count == 0) {
        int num_defaults = sizeof(default_bookmarks) / sizeof(default_bookmarks[0]);
        for (int i = 0; i < num_defaults && *count < max; i++) {
            const char* entry = default_bookmarks[i];
            const char* sep = strchr(entry, '|');
            if (sep) {
                int tlen = sep - entry;
                strncpy(bookmarks[*count].title, entry, tlen);
                bookmarks[*count].title[tlen] = 0;
                strncpy(bookmarks[*count].url, sep + 1, MAX_URL_LEN - 1);
                (*count)++;
            }
        }
    }
}

void bookmarks_save(const Bookmark* bookmarks, int count) {
    mkdir(BOOKMARKS_DIR, 0777);
    FILE* f = fopen(BOOKMARKS_PATH, "w");
    if (!f) return;
    for (int i = 0; i < count; i++) {
        fprintf(f, "%s|%s\n", bookmarks[i].title, bookmarks[i].url);
    }
    fclose(f);
}

bool bookmarks_add(Bookmark* bookmarks, int* count, int max, const char* url, const char* title) {
    if (*count >= max) return false;
    strncpy(bookmarks[*count].title, title && title[0] ? title : "Untitled", MAX_TEXT_LEN - 1);
    strncpy(bookmarks[*count].url, url, MAX_URL_LEN - 1);
    (*count)++;
    bookmarks_save(bookmarks, *count);
    return true;
}

bool bookmarks_remove(Bookmark* bookmarks, int* count, int index) {
    if (index < 0 || index >= *count) return false;
    for (int i = index; i < *count - 1; i++) {
        bookmarks[i] = bookmarks[i + 1];
    }
    (*count)--;
    bookmarks_save(bookmarks, *count);
    return true;
}
