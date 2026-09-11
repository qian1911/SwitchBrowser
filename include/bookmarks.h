#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include "types.h"

void bookmarks_load(Bookmark* bookmarks, int* count, int max);
void bookmarks_save(const Bookmark* bookmarks, int count);
bool bookmarks_add(Bookmark* bookmarks, int* count, int max, const char* url, const char* title);
bool bookmarks_remove(Bookmark* bookmarks, int* count, int index);

#endif
