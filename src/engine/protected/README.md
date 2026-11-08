# Readable versions of protector-obfuscated functions

The functions here are mutated or virtualised by the protector in the original image, so they cannot be
byte-verified like the rest of `src/engine`. They are written from what survives: the code around each
hidden call, the strings and globals they use, their callers, and emulation traces of the original.

* `files.cpp`  timer (`time_now`) and the profile-folder file helpers (`settings_path` .. `write_text_file`)
* `input.cpp`  GetModuleHandle/GetProcAddress, the game-window input hook (`hook_game_window`, `game_window_proc`,
  `restore_game_window`), synthetic clicks (`post_synthetic_click`) and the unload thread (`start_unload_thread`, `unload_thread`)
* `cjson.cpp`  cJSON parse_value / print_value / get_object_item (upstream cJSON 1.7.x)
* `hooks.cpp`  the `getMouseOver` native hook (`hook_get_mouse_over`), plus notes on `set_java_hooks` and the CRT DLL entry

Windows APIs are named from their arguments and use; places marked "inferred" or "NOT RECOVERED" say so.
They are compile-checked by `build.sh` (clang-cl, see include/slinky/win32.h) but excluded from the
byte comparison, and the image's protected copies stay listed in build/stubbed.json.
