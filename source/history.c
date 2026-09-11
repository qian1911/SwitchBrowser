#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <switch.h>

#include "history.h"
#include "types.h"

static void ensure_dir(void) {
    mkdir("sdmc:/switch/SwitchBrowser", 0777);
}

void history_load(HistoryEntry *history, int *count, int max) {
    *count = 0;

    FILE *f = fopen(HISTORY_FILE, "r");
    if (!f) return;

    char line[MAX_URL_LEN + MAX_TITLE_LEN + 24];
    while (*count < max && fgets(line, sizeof(line), f)) {
        u64 ts = 0;
        char url[MAX_URL_LEN];
        char title[MAX_TITLE_LEN];

        // Format: timestamp|url|title
        if (sscanf(line, "%llu|%2047[^|]|%255[^\n]", &ts, url, title) == 3) {
            history[*count].timestamp = ts;
            strncpy(history[*count].url, url, MAX_URL_LEN - 1);
            history[*count].url[MAX_URL_LEN - 1] = '\0';
            strncpy(history[*count].title, title, MAX_TITLE_LEN - 1);
            history[*count].title[MAX_TITLE_LEN - 1] = '\0';
            (*count)++;
        }
    }
    fclose(f);
}

void history_save(const HistoryEntry *history, int count) {
    ensure_dir();
    FILE *f = fopen(HISTORY_FILE, "w");
    if (!f) return;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%llu|%s|%s\n",
                (unsigned long long)history[i].timestamp,
                history[i].url,
                history[i].title);
    }
    fclose(f);
}

bool history_add(HistoryEntry *history, int *count, int max, const char *url, const char *title) {
    if (!url || !url[0]) return false;

    // Don't add duplicate consecutive entries
    if (*count > 0 && strncmp(history[*count - 1].url, url, MAX_URL_LEN) == 0)
        return false;

    if (*count >= max) {
        // Shift array — remove oldest entry
        memmove(history, history + 1, (max - 1) * sizeof(HistoryEntry));
        (*count)--;
    }

    strncpy(history[*count].url, url, MAX_URL_LEN - 1);
    history[*count].url[MAX_URL_LEN - 1] = '\0';

    if (title && title[0]) {
        strncpy(history[*count].title, title, MAX_TITLE_LEN - 1);
    } else {
        history[*count].title[0] = '\0';
    }
    history[*count].title[MAX_TITLE_LEN - 1] = '\0';

    history[*count].timestamp = (u64)time(NULL);
    (*count)++;

    history_save(history, *count);
    return true;
}

void history_clear(HistoryEntry *history, int *count) {
    *count = 0;
    ensure_dir();
    FILE *f = fopen(HISTORY_FILE, "w");
    if (f) fclose(f);
}
