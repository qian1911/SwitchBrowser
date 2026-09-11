#include "browser.h"
#include <switch.h>
#include <string.h>
#include <stdio.h>

bool browser_check_applet_mode(void) {
    AppletType at = appletGetAppletType();
    return (at != AppletType_Application);
}

bool browser_is_url(const char* text) {
    if (!text) return false;
    if (strncmp(text, "http://", 7) == 0) return true;
    if (strncmp(text, "https://", 8) == 0) return true;
    const char* dot = strchr(text, '.');
    if (dot && dot != text && dot[1] != '\0' && dot[1] != '/') return true;
    return false;
}

void browser_normalize_url(char* url, size_t size) {
    if (!url || !url[0]) return;
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0)
        return;
    if (strchr(url, '.') && !strchr(url, ' ')) {
        char tmp[MAX_URL_LEN];
        snprintf(tmp, sizeof(tmp), "https://%s", url);
        strncpy(url, tmp, size - 1);
        url[size - 1] = 0;
    }
}

void browser_build_search_url(char* out, size_t out_size, const char* query, const char* engine) {
    if (!engine || !engine[0])
        engine = "https://www.baidu.com/s?wd=";

    char encoded[1024];
    int ei = 0;
    for (int i = 0; query[i] && ei < 1020; i++) {
        char c = query[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[ei++] = c;
        } else if (c == ' ') {
            encoded[ei++] = '+';
        } else {
            ei += snprintf(encoded + ei, sizeof(encoded) - ei, "%%%02X", (unsigned char)c);
        }
    }
    encoded[ei] = 0;
    snprintf(out, out_size, "%s%s", engine, encoded);
}

bool browser_input_text(char* out_text, size_t out_size, const char* header, const char* initial) {
    SwkbdConfig swkbd;
    Result rc;
    memset(out_text, 0, out_size);

    rc = swkbdCreate(&swkbd, 0);
    if (R_FAILED(rc)) return false;

    swkbdConfigMakePresetDefault(&swkbd);
    if (header) swkbdConfigSetHeaderText(&swkbd, header);
    if (initial) swkbdConfigSetInitialText(&swkbd, initial);
    swkbdConfigSetStringLenMax(&swkbd, out_size > 1 ? (u32)(out_size - 1) : 1);

    rc = swkbdShow(&swkbd, out_text, out_size);
    swkbdClose(&swkbd);

    return R_SUCCEEDED(rc) && out_text[0] != 0;
}

bool browser_input_url(char* out_url, size_t out_size, const char* initial_text) {
    return browser_input_text(out_url, out_size, "Enter URL or Search", initial_text ? initial_text : "https://");
}
