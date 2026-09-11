#ifndef BROWSER_H
#define BROWSER_H

#include "types.h"

bool browser_check_applet_mode(void);
bool browser_input_url(char* out_url, size_t out_size, const char* initial_text);
bool browser_input_text(char* out_text, size_t out_size, const char* header, const char* initial);
void browser_normalize_url(char* url, size_t size);
void browser_build_search_url(char* out, size_t out_size, const char* query, const char* engine);
bool browser_is_url(const char* text);

#endif
