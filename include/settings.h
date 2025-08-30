#ifndef SETTINGS_H
#define SETTINGS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool line_numbers;
    bool show_whitespace;
    bool auto_indent;
    bool tab_as_spaces;
    char font_path[256];
    char theme[64];
    float font_size;
    uint8_t tab_size;
} Settings;

void settings_init(Settings *settings);
void settings_load(const char *path);
void settings_save(const char *path);

#endif // SETTINGS_H