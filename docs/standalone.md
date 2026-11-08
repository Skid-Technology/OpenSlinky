# Standalone build: what is left (measured 2026-10-30)

State of the source: every function of the client is now C++ except the 43 protector-mutated ones, which have
readable versions in `src/engine/protected/` (17 of 20 used ones; see README there). Counts:

| layer | functions | in C++ | byte-exact |
|---|---:|---:|---:|
| module settings (cb/reg/aux) | 146 | 146 | 145 |
| module logic | 200 | 200 | 200 |
| engine (`manifest_engine.json`) | 880 | 837 (+17 readable protected) | 591 |

`tools/extract_engine.py` now also finds functions nothing calls directly (the protector's virtualised init block
starts them through function pointers): the input handler, JVMTI unload, bytecode patcher, GLFW loader, key maps,
JNI bootstrap, allocators, destructors.

## What a standalone (no original image) build still needs

Undefined symbols over all 440 compiled objects (`llvm-nm`):

| kind | count | what it is | plan |
|---|---:|---|---|
| `rdata_*` | 2161 | strings, float/double constants, lookup tables, vtable-like arrays | generate `src/data/rdata_*.cpp` from the image, typed by use (strings as literals, floats, structs where known) |
| `data_*` | 1352 | globals: settings values, module descriptors, state | generate definitions with initial values from the image's `.data` |
| `fn_*` | 97 | NanoVG + its GL3 backend (0x62850+; not ImGui), MSVC CRT (0xab000+), import thunks | link upstream NanoVG (nanovg_gl, GL3), the real CRT; map thunks to real APIs |
| `W_*`/`R_*`/`log_fmt` | 48 | the widget/registration API the module code calls | already written as headers in include/slinky/api.h; needs real bodies (they are `fn_*` in the image) |
| `__imp_*` | - | Win32 imports used by src/engine/protected | link kernel32 (Windows) / replace in a platform layer (Linux) |

Platform layer (needed for Linux): the Windows-only pieces are the window-procedure input hook and click injection
(`protected/input.cpp`), file/path helpers (`protected/files.cpp`), `GetModuleHandle/GetProcAddress`, DLL entry/unload.
Everything else goes through JNI/JVMTI (portable) and OpenGL/NanoVG (portable). On Linux: load as a JVMTI agent
(`-agentpath`), take input from GLFW callbacks (`init_glfw_entrypoints` already resolves them) or X11, use `dlopen/dlsym`.

## Order
1. generate data/rdata definitions (largest mechanical step, enables linking the C++ without the image)
2. real NanoVG + CRT, widget API bodies
3. CMake: Windows DLL first (comparable against the original), then Linux `.so` with the platform layer
4. rewrite the unrecovered virtualised blocks (one-time init in `hook_get_mouse_over`, hook on/off `set_java_hooks`) with our own design

## Result of the data step (tools/gen_data.py, tools/gen_aliases.py, tools/link_probe.sh)

* `src/data/rdata.s` (1.6 MB, 4430 relocated pointers, 6937 strings) and `src/data/data.s` (2595 pointers) are generated
  from the image and verified byte-for-byte by `tools/check_data.py` (0 differing bytes, every pointer target correct).
  Strings are `.asciz`, pointers are symbol references, 4/8-byte constants carry their float/double value as a comment.
* `src/engine/aliases.cpp` maps engine calls of logic functions (`gui_handle_input_event_entry` -> `gui_handle_input_event`) and the module API names
  (`W_bool` -> `widget_bool`, ...).
* `tools/link_probe.sh` links every C++ object with these into a DLL (no CRT, no imports). Before: 3513 data symbols and
  ~700 functions undefined. Now: 300 undefined symbols, 0 duplicates, and all of them are library code or platform API:
  199 MSVC CRT functions, 42 NanoVG functions, 23 import thunks, ~25 Win32 imports, ~9 libc/runtime symbols, and
  one piece of client code (`set_java_hooks`, the unrecovered hook on/off).
* So the client's own code is closed: everything it needs besides NanoVG, the C runtime and the OS is in this tree.

## Status update: the DLL builds (CMakeLists.txt)

* `cmake -S . -B build/cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-clang-cl.cmake && cmake --build build/cmake`
  produces `build/cmake/slinky.dll` without the original image and without a Windows SDK.
* What stood open in the table above is now bound: the C runtime and NanoVG by `src/platform/` (CRT shim, NanoVG
  names, GL slots), the OS imports by import libraries generated from `cmake/imports/*.def`, the few libc declarations
  the bundled C code needs by `third_party/minicrt/`.
* Start-up, hook control, teardown and the first-frame GUI setup are new code (`docs/init.md`).
* Still open: running it in a game (nothing has been run), the Linux platform layer, and the items marked inferred in
  `docs/init.md`.
