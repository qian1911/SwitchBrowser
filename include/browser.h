#pragma once
#include "types.h"

/// Initialize web services (socket, applet)
void browser_init(void);

/// Launch the built-in Switch web browser applet with the given URL.
/// Returns the last visited URL in last_url (if non-NULL).
/// Returns true if browsing completed successfully.
bool browser_navigate(const char *url, char *last_url, size_t last_url_size);

/// Show software keyboard for URL input.
/// Returns true if user entered text and pressed OK.
bool browser_input_url(char *out_url, size_t out_size, const char *initial_text);

/// Show software keyboard for generic text input.
bool browser_input_text(char *out_text, size_t out_size, const char *header, const char *initial);

/// Ensure URL has http:// or https:// prefix.
void browser_normalize_url(char *url, size_t size);

/// Build a search URL from a query string.
void browser_build_search_url(char *out, size_t out_size, const char *query, const char *engine);
