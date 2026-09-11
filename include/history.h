#pragma once
#include "types.h"

/// Load browsing history from SD card.
void history_load(HistoryEntry *history, int *count, int max);

/// Save browsing history to SD card.
void history_save(const HistoryEntry *history, int count);

/// Add a history entry. Returns true if added.
bool history_add(HistoryEntry *history, int *count, int max, const char *url, const char *title);

/// Clear all history.
void history_clear(HistoryEntry *history, int *count);
