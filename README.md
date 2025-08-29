# Dream Editor for TExt Yo

**DETEY** - is a small, text editor by Tsoding (with my improvements). 
Syntax highlighting, minimalistic file browser and GPU-backed text rendering.
It is implemented in portable C and uses SDL2, FreeType and GLEW for windowing, font rasterization and OpenGL.

## Highlights
- Fast glyph atlas based text rendering (see [`free_glyph_atlas_render_line_sized`](src/free_glyph.c))
- Simple software lexer for basic C/C++ token highlighting (see [`lexer_next`](src/lexer.c))
- Camera-driven UI with a tiny renderer abstraction (see [`simple_renderer_init`](src/simple_renderer.c))
- Editor core with selection, search, file IO and cursor movement (see [`editor_render`](src/editor.c), [`editor_save`](src/editor.c))
- Minimal file browser implemented in [src/file_browser.c](src/file_browser.c)

## Quick start

POSIX:
```sh
./build.sh
./ded src/main.c
```

Windows (MSVC):
```bat
.\setup_dependencies.bat
.\build_msvc.bat
.\ded.exe src\main.c
```

Code overview (entry points)
- [src/main.c](src/main.c) — application bootstrap, event loop and keybindings
- [src/editor.c](src/editor.c) — editor logic, rendering glue and user actions
- [src/lexer.c](src/lexer.c) — tokenization / syntax classification
- [src/free_glyph.c](src/free_glyph.c) — glyph atlas and text drawing helpers
- [src/simple_renderer.c](src/simple_renderer.c) — thin GL renderer and shader management
- [src/file_browser.c](src/file_browser.c) — simple directory listing and navigation

Contributing
- Bug fixes and small documentation updates welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

License
- MIT — see [LICENSE](LICENSE)