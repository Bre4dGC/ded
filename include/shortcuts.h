#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <SDL2/SDL.h>

#include "editor.h"
#include "file_browser.h"
#include "simple_renderer.h"
#include "free_glyph.h"
#include "common.h"

static inline void shortcuts_handle_keydown(SDL_Event *event,
                                            bool *file_browser,
                                            Editor *editor,
                                            File_Browser *fb,
                                            Simple_Renderer *sr,
                                            Free_Glyph_Atlas *atlas,
                                            Errno *err)
{
    SDL_Keycode sym = event->key.keysym.sym;
    Uint16 mod = event->key.keysym.mod;

    if (*file_browser)
    {
        switch (sym)
        {
            case SDLK_d:
                if (mod & KMOD_CTRL) {
                    *file_browser = false;
                }
                return;
            case SDLK_UP:
                if (fb->cursor > 0) fb->cursor -= 1;
                return;
            case SDLK_DOWN:
                if (fb->cursor + 1 < fb->files.count) fb->cursor += 1;
                return;
            case SDLK_RETURN:
            {
                const char *file_path = fb_file_path(fb);
                if (!file_path) return;
                File_Type ft;
                *err = type_of_file(file_path, &ft);
                if (*err != 0) {
                    fprintf(stderr, "Could not determine type of file %s: %s\n", file_path, strerror(*err));
                    return;
                }
                switch (ft) {
                    case FT_DIRECTORY:
                        *err = fb_change_dir(fb);
                        if (*err != 0) {
                            fprintf(stderr, "Could not change directory to %s: %s\n", file_path, strerror(*err));
                        }
                        break;
                    case FT_REGULAR:
                        *err = editor_load_from_file(editor, file_path);
                        if (*err != 0) {
                            fprintf(stderr, "Could not open file %s: %s\n", file_path, strerror(*err));
                        } else {
                            *file_browser = false;
                        }
                        break;
                    case FT_OTHER:
                    default:
                        fprintf(stderr, "%s is neither a regular file nor a directory. We can't open it.\n", file_path);
                        break;
                }
                return;
            }
            default:
                return;
        }
    }

    /* editor mode */
    switch (sym)
    {
        case SDLK_HOME:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_to_begin(editor);
            else editor_move_to_line_begin(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        case SDLK_END:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_to_end(editor);
            else editor_move_to_line_end(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        case SDLK_BACKSPACE:
            if(mod & KMOD_CTRL) {
                editor_delete_word_left(editor);
            }
            else {
                editor_backspace(editor);
                editor->last_stroke = SDL_GetTicks();
            }
            break;

        case SDLK_s:
            if (mod & KMOD_CTRL) {
                if (editor->file_path.count > 0) {
                    *err = editor_save(editor);
                    if (*err != 0) {
                        fprintf(stderr, "Could not save currently edited file: %s\n", strerror(*err));
                    }
                } else {
                    fprintf(stderr, "Nowhere to save the text\n");
                }
            }
            break;

        case SDLK_d:
            if (mod & KMOD_CTRL) {
                *file_browser = true;
            }
            break;

        case SDLK_z:
            if (mod & KMOD_CTRL) {
                // TODO: implement undo
            }
            break;

        case SDLK_y:
            if (mod & KMOD_CTRL) {
                // TODO: implement redo
            }
            break;

        case SDLK_COMMA:
            if (mod & KMOD_CTRL) {
                // TODO: open settings
            }
            break;

        case SDLK_n:
            if (mod & KMOD_CTRL) {
                // TODO: create a new file
            }
            break;

        // TODO: Alt+Up/Down to move lines up and down
        // TODO: Ctrl+[/] to fold/unfold code blocks

        case SDLK_F5:
            simple_renderer_reload_shaders(sr);
            break;

        case SDLK_RETURN:
            if (editor->searching) editor_stop_search(editor);
            else {
                editor_insert_char(editor, '\n');
                editor->last_stroke = SDL_GetTicks();
            }
            break;

        case SDLK_DELETE:
            if (mod & KMOD_CTRL) {
                editor_delete_word_right(editor);
            }
            else {
                editor_delete(editor);
                editor->last_stroke = SDL_GetTicks();
            }
            break;

        case SDLK_f:
            if (mod & KMOD_CTRL) {
                editor_start_search(editor);
            }
            break;

        case SDLK_ESCAPE:
            editor_stop_search(editor);
            editor_update_selection(editor, mod & KMOD_SHIFT);
            break;

        case SDLK_a:
            if (mod & KMOD_CTRL) {
                editor->selection = true;
                editor->select_begin = 0;
                editor->cursor = editor->data.count;
            }
            break;

        case SDLK_TAB:
            for (size_t i = 0; i < 4; ++i) {
                editor_insert_char(editor, ' ');
            }
            break;

        case SDLK_c:
            if (mod & KMOD_CTRL) editor_clipboard_copy(editor);
            break;

        case SDLK_v:
            if (mod & KMOD_CTRL) editor_clipboard_paste(editor);
            break;

        case SDLK_UP:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_paragraph_up(editor);
            else editor_move_line_up(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        case SDLK_DOWN:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_paragraph_down(editor);
            else editor_move_line_down(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        case SDLK_LEFT:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_word_left(editor);
            else editor_move_char_left(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        case SDLK_RIGHT:
            editor_update_selection(editor, mod & KMOD_SHIFT);
            if (mod & KMOD_CTRL) editor_move_word_right(editor);
            else editor_move_char_right(editor);
            editor->last_stroke = SDL_GetTicks();
            break;

        default: break;
    }
}

#endif // SHORTCUTS_H