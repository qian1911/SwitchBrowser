#pragma once
#include "types.h"

/// Load bookmarks from SD card.
void bookmarks_load(Bookmark *bookmarks, int *count, int max);

/// Save bookmarks to SD card.
void bookmarks_save(const Bookmark *bookmarks, int count);

/// Add a bookmark. Returns true if added (or already exists).
bool bookmarks_add(Bookmark *bookmarks, int *count, int max, const char *url, const char *title);

/// Remove a bookmark by index. Returns true if removed.
bool bookmarks_remove(Bookmark *bookmarks, int *count, int index);

/// Check if a URL is already bookmarked. Returns index or -1.
int bookmarks_find(const Bookmark *bookmarks, int count, const char *url);

/// Get default/preset bookmarks (popular sites).
void bookmarks_get_defaults(Bookmark *bookmarks, int *count, int max);
