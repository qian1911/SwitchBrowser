#ifndef WEB_FETCH_H
#define WEB_FETCH_H

#include "types.h"

bool web_fetch_page(const char* url, PageContent* page);
void html_parse(const char* html, size_t size, PageContent* page);

#endif
