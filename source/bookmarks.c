#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <switch.h>

#include "bookmarks.h"
#include "types.h"

static void ensure_dir(void) {
    mkdir("sdmc:/switch/SwitchBrowser", 0777);
}

void bookmarks_load(Bookmark *bookmarks, int *count, int max) {
    *count = 0;

    FILE *f = fopen(BOOKMARKS_FILE, "r");
    if (!f) {
        bookmarks_get_defaults(bookmarks, count, max);
        return;
    }

    char line[MAX_URL_LEN + MAX_TITLE_LEN + 2];
    while (*count < max && fgets(line, sizeof(line), f)) {
        // Format: URL|Title
        char *sep = strchr(line, '|');
        if (sep) {
            *sep = '\0';
            // Remove trailing newline
            char *nl = strchr(sep + 1, '\n');
            if (nl) *nl = '\0';

            strncpy(bookmarks[*count].url, line, MAX_URL_LEN - 1);
            bookmarks[*count].url[MAX_URL_LEN - 1] = '\0';
            strncpy(bookmarks[*count].title, sep + 1, MAX_TITLE_LEN - 1);
            bookmarks[*count].title[MAX_TITLE_LEN - 1] = '\0';
            (*count)++;
        }
    }
    fclose(f);
}

void bookmarks_save(const Bookmark *bookmarks, int count) {
    ensure_dir();
    FILE *f = fopen(BOOKMARKS_FILE, "w");
    if (!f) return;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s|%s\n", bookmarks[i].url, bookmarks[i].title);
    }
    fclose(f);
}

bool bookmarks_add(Bookmark *bookmarks, int *count, int max, const char *url, const char *title) {
    if (!url || !url[0]) return false;

    // Check if already exists
    if (bookmarks_find(bookmarks, *count, url) >= 0)
        return true;

    if (*count >= max) return false;

    strncpy(bookmarks[*count].url, url, MAX_URL_LEN - 1);
    bookmarks[*count].url[MAX_URL_LEN - 1] = '\0';

    if (title && title[0]) {
        strncpy(bookmarks[*count].title, title, MAX_TITLE_LEN - 1);
    } else {
        // Use URL domain as title
        const char *start = url;
        if (strncmp(url, "https://", 8) == 0) start = url + 8;
        else if (strncmp(url, "http://", 7) == 0) start = url + 7;
        strncpy(bookmarks[*count].title, start, MAX_TITLE_LEN - 1);
    }
    bookmarks[*count].title[MAX_TITLE_LEN - 1] = '\0';
    (*count)++;

    bookmarks_save(bookmarks, *count);
    return true;
}

bool bookmarks_remove(Bookmark *bookmarks, int *count, int index) {
    if (index < 0 || index >= *count) return false;

    for (int i = index; i < *count - 1; i++) {
        memcpy(&bookmarks[i], &bookmarks[i + 1], sizeof(Bookmark));
    }
    (*count)--;

    bookmarks_save(bookmarks, *count);
    return true;
}

int bookmarks_find(const Bookmark *bookmarks, int count, const char *url) {
    for (int i = 0; i < count; i++) {
        if (strncmp(bookmarks[i].url, url, MAX_URL_LEN) == 0)
            return i;
    }
    return -1;
}

void bookmarks_get_defaults(Bookmark *bookmarks, int *count, int max) {
    struct { const char *url; const char *title; } defaults[] = {
        {"https://www.google.com",            "Google"},
        {"https://www.bing.com",              "Bing"},
        {"https://www.baidu.com",             "Baidu"},
        {"https://www.youtube.com",           "YouTube"},
        {"https://www.wikipedia.org",        "Wikipedia"},
        {"https://www.github.com",           "GitHub"},
        {"https://www.reddit.com",           "Reddit"},
        {"https://www.bilibili.com",         "Bilibili"},
        {"https://www.duckduckgo.com",       "DuckDuckGo"},
        {"https://news.ycombinator.com",     "Hacker News"},
    };
    int n = sizeof(defaults) / sizeof(defaults[0]);
    if (n > max) n = max;

    for (int i = 0; i < n; i++) {
        strncpy(bookmarks[i].url, defaults[i].url, MAX_URL_LEN - 1);
        bookmarks[i].url[MAX_URL_LEN - 1] = '\0';
        strncpy(bookmarks[i].title, defaults[i].title, MAX_TITLE_LEN - 1);
        bookmarks[i].title[MAX_TITLE_LEN - 1] = '\0';
    }
    *count = n;

    bookmarks_save(bookmarks, *count);
}
