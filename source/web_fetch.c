#include "web_fetch.h"
#include <curl/curl.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    char* data;
    size_t size;
} CurlBuffer;

static const char* stristr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    size_t hl = strlen(haystack), nl = strlen(needle);
    if (nl == 0) return haystack;
    if (hl < nl) return NULL;
    for (size_t i = 0; i <= hl - nl; i++) {
        bool match = true;
        for (size_t j = 0; j < nl; j++) {
            if (tolower((unsigned char)haystack[i+j]) != tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return haystack + i;
    }
    return NULL;
}

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    CurlBuffer* buf = (CurlBuffer*)userdata;
    size_t total = size * nmemb;
    char* new_data = realloc(buf->data, buf->size + total + 1);
    if (!new_data) return 0;
    buf->data = new_data;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = 0;
    return total;
}

static void html_decode_entities(char* text, size_t len) {
    char* src = text;
    char* dst = text;
    while (*src && (size_t)(dst - text) < len - 1) {
        if (*src == '&') {
            if (strncmp(src, "&amp;", 5) == 0) { *dst++ = '&'; src += 5; }
            else if (strncmp(src, "&lt;", 4) == 0) { *dst++ = '<'; src += 4; }
            else if (strncmp(src, "&gt;", 4) == 0) { *dst++ = '>'; src += 4; }
            else if (strncmp(src, "&quot;", 6) == 0) { *dst++ = '"'; src += 6; }
            else if (strncmp(src, "&#39;", 5) == 0) { *dst++ = '\''; src += 5; }
            else if (strncmp(src, "&nbsp;", 6) == 0) { *dst++ = ' '; src += 6; }
            else { *dst++ = *src++; }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static void strip_whitespace(char* s) {
    char* start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = 0;
    }
    char* dst = s;
    for (char* src = s; *src; src++) {
        if (isspace((unsigned char)*src)) {
            *dst++ = ' ';
            while (*src && isspace((unsigned char)*src)) src++;
            src--;
        } else {
            *dst++ = *src;
        }
    }
    *dst = 0;
}

static bool is_block_tag(const char* tag) {
    return strcasecmp(tag, "p") == 0 || strcasecmp(tag, "div") == 0 ||
           strcasecmp(tag, "br") == 0 || strcasecmp(tag, "h1") == 0 ||
           strcasecmp(tag, "h2") == 0 || strcasecmp(tag, "h3") == 0 ||
           strcasecmp(tag, "h4") == 0 || strcasecmp(tag, "h5") == 0 ||
           strcasecmp(tag, "h6") == 0 || strcasecmp(tag, "li") == 0 ||
           strcasecmp(tag, "tr") == 0 || strcasecmp(tag, "table") == 0 ||
           strcasecmp(tag, "ul") == 0 || strcasecmp(tag, "ol") == 0 ||
           strcasecmp(tag, "section") == 0 || strcasecmp(tag, "article") == 0 ||
           strcasecmp(tag, "header") == 0 || strcasecmp(tag, "footer") == 0;
}

void html_parse(const char* html, size_t size, PageContent* page) {
    page->line_count = 0;
    page->link_count = 0;
    page->title[0] = 0;
    page->error_msg[0] = 0;

    bool in_tag = false;
    bool in_script = false;
    bool in_style = false;
    bool in_title = false;
    char tag_name[256];
    int tag_pos = 0;
    char text_buf[4096];
    int text_pos = 0;
    char link_url[MAX_URL_LEN];
    int link_pos = 0;
    bool in_link = false;
    int line_start = 0;

    for (size_t i = 0; i < size && page->line_count < MAX_PAGE_LINES; i++) {
        char c = html[i];

        if (!in_tag && !in_script && !in_style) {
            if (c == '<') {
                if (text_pos > 0) {
                    text_buf[text_pos] = 0;
                    html_decode_entities(text_buf, sizeof(text_buf));
                    strip_whitespace(text_buf);
                    if (text_buf[0] && strlen(text_buf) > 1) {
                        if (page->line_count < MAX_PAGE_LINES) {
                            strncpy(page->lines[page->line_count].text, text_buf, MAX_LINE_LEN - 1);
                            page->lines[page->line_count].text[MAX_LINE_LEN - 1] = 0;
                            page->lines[page->line_count].link_idx = in_link ? page->link_count - 1 : -1;
                            page->line_count++;
                        }
                    }
                    text_pos = 0;
                }
                in_tag = true;
                tag_pos = 0;
                if (i + 1 < size && html[i + 1] == '/') {
                    tag_name[0] = '/';
                    tag_pos = 1;
                    i++;
                }
            } else {
                if (text_pos < (int)sizeof(text_buf) - 1) {
                    text_buf[text_pos++] = c;
                }
            }
        } else if (in_tag) {
            if (c == '>') {
                in_tag = false;
                tag_name[tag_pos] = 0;

                char lower_tag[256];
                for (int j = 0; j < tag_pos && j < 255; j++)
                    lower_tag[j] = tolower((unsigned char)tag_name[j]);
                lower_tag[tag_pos < 255 ? tag_pos : 255] = 0;

                char* space = strchr(lower_tag, ' ');
                if (space) *space = 0;

                if (strcmp(lower_tag, "script") == 0) in_script = true;
                else if (strcmp(lower_tag, "style") == 0) in_style = true;
                else if (strcmp(lower_tag, "/script") == 0) in_script = false;
                else if (strcmp(lower_tag, "/style") == 0) in_style = false;
                else if (strcmp(lower_tag, "title") == 0) in_title = true;
                else if (strcmp(lower_tag, "/title") == 0) in_title = false;
                else if (strcmp(lower_tag, "a") == 0 || strncmp(lower_tag, "a ", 2) == 0) {
                    in_link = true;
                    const char* href = stristr(tag_name, "href=");
                    if (href) {
                        href += 5;
                        if (*href == '"' || *href == '\'') href++;
                        link_pos = 0;
                        while (*href && *href != '"' && *href != '\'' && *href != ' ' && link_pos < MAX_URL_LEN - 1) {
                            link_url[link_pos++] = *href++;
                        }
                        link_url[link_pos] = 0;
                        if (link_pos > 0 && page->link_count < MAX_LINKS) {
                            strncpy(page->links[page->link_count].url, link_url, MAX_URL_LEN - 1);
                            page->links[page->link_count].url[MAX_URL_LEN - 1] = 0;
                            page->links[page->link_count].text[0] = 0;
                            page->link_count++;
                        }
                    }
                }
                else if (strcmp(lower_tag, "/a") == 0) {
                    in_link = false;
                    link_url[0] = 0;
                }
                else if (is_block_tag(lower_tag) || strncmp(lower_tag, "br", 2) == 0) {
                    if (text_pos > 0) {
                        text_buf[text_pos] = 0;
                        html_decode_entities(text_buf, sizeof(text_buf));
                        strip_whitespace(text_buf);
                        if (text_buf[0] && strlen(text_buf) > 1 && page->line_count < MAX_PAGE_LINES) {
                            strncpy(page->lines[page->line_count].text, text_buf, MAX_LINE_LEN - 1);
                            page->lines[page->line_count].text[MAX_LINE_LEN - 1] = 0;
                            page->lines[page->line_count].link_idx = in_link ? (page->link_count - 1) : -1;
                            page->line_count++;
                        }
                        text_pos = 0;
                    }
                }
            } else {
                if (tag_pos < 255) {
                    tag_name[tag_pos++] = c;
                }
            }
        } else if (in_script) {
            if (c == '<' && i + 8 < size && strncasecmp(html + i, "</script>", 9) == 0) {
                in_script = false;
                i += 8;
            }
        } else if (in_style) {
            if (c == '<' && i + 7 < size && strncasecmp(html + i, "</style>", 8) == 0) {
                in_style = false;
                i += 7;
            }
        }

        if (in_title && !in_tag && c != '>') {
            size_t tlen = strlen(page->title);
            if (tlen < MAX_TEXT_LEN - 1) {
                page->title[tlen] = c;
                page->title[tlen + 1] = 0;
            }
        }
    }

    if (text_pos > 0 && page->line_count < MAX_PAGE_LINES) {
        text_buf[text_pos] = 0;
        html_decode_entities(text_buf, sizeof(text_buf));
        strip_whitespace(text_buf);
        if (text_buf[0] && strlen(text_buf) > 1) {
            strncpy(page->lines[page->line_count].text, text_buf, MAX_LINE_LEN - 1);
            page->lines[page->line_count].text[MAX_LINE_LEN - 1] = 0;
            page->lines[page->line_count].link_idx = -1;
            page->line_count++;
        }
    }

    strip_whitespace(page->title);
    if (!page->title[0]) {
        strncpy(page->title, "Untitled Page", MAX_TEXT_LEN - 1);
    }
}

bool web_fetch_page(const char* url, PageContent* page) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        strncpy(page->error_msg, "Failed to init curl", MAX_TEXT_LEN - 1);
        return false;
    }

    CurlBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Switch; homebrew) SwitchBrowser/2.0");
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 65536L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    page->http_status = (int)http_code;

    bool ok = false;
    if (res == CURLE_OK && buf.size > 0) {
        html_parse(buf.data, buf.size, page);
        ok = true;
    } else {
        snprintf(page->error_msg, MAX_TEXT_LEN, "curl error: %s", curl_easy_strerror(res));
    }

    free(buf.data);
    curl_easy_cleanup(curl);
    return ok;
}
