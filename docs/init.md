# Start-up, hooks and shutdown

How the client gets from "DLL loaded into the JVM" to "game calls into the modules", and back. Most of the original
start-up is protector-virtualised, so it could not be translated; it is written as new code
(`src/engine/protected/init.cpp`, `src/platform/win/entry.cpp`) from what the surrounding readable code expects.
Facts below come from the image unless marked **inferred**. The licence check is not part of this client.

## What was found where

| Piece | Function | State |
|---|---|---|
| Class-hook installer | `install_class_hooks` | **real code**, was missed by the first gap scan. Rewritten readably (tables are data), checked against the original by running both (`tools/emu_install.py`) |
| Class-hook remover | `remove_class_hooks` | real code, translated exactly |
| `ClassFileLoadHook` + class rewriter | `class_file_load_hook`, `rewrite_class_file`, `rewrite_instruction`, ... | real code, translated |
| `SlinkyChannelHandler.onPacket` native | `on_packet` | real code (also missed by the scan), translated |
| Key handlers for profile / module binds | `on_profile_bind`, `on_module_bind`, `bind_profile_to_key` | real code (missed), translated |
| `JNI_GetCreatedJavaVMs` lookup | `get_java_vm` | real code (missed), translated |
| Hooks on/off | `set_java_hooks` | virtualised -> `init.cpp` (**inferred**: GetEnv JVMTI, install or remove, log failure) |
| Start-up thread body | inside the original image's start-up routine at RVA 0x1000 | virtualised -> `slinky_setup` in `init.cpp` (**inferred** order) |
| First-frame GUI setup | inside `hook_get_mouse_over` | virtualised -> `gui_init` in `protected/hooks.cpp` (**inferred**) |
| Global teardown | DLL detach | virtualised -> `slinky_teardown` (**inferred**) |

The first gap scan only accepted a function that follows `0xcc/0x90/0xc3`, so functions after multi-byte NOP padding
or directly after a tail `jmp` were skipped; 33 real functions (7 KB) were found by fixing that
(`tools/extract_engine.py`). 43 protector stubs remain; `empty_placeholder` is still an empty placeholder (it has no callers).

## Entry (Windows)

`DllMain` (`src/platform/win/entry.cpp`) starts a thread on `DLL_PROCESS_ATTACH`; that thread runs `slinky_setup`.
On failure it prints `Initialization failed: <code>` and starts the unload thread (`start_unload_thread`).

## `slinky_setup` (init.cpp)

1. `get_java_vm`: JavaVM from `jvm.dll` -> `init_javavm`; attach the thread.
2. JNI ID table: `calloc(JIds)` -> `java_natives_world_cached_jni_ids`, `init_jni_ids` (game class loader, mapping detection, every class/method/field
   ID). Mapping: `ids->variant` 0 = 1.7.10, 1 = 1.8.9; `ids->names` 0 = obfuscated, 1 = MCP, 2 = SRG.
3. Module manager: `create_module_manager(java_natives_world_module_manager, set_module_bind)` builds the lists from the static module list (67 descriptors);
   `set_module_bind` is the key-bind setter it stores. Immediately afterwards `save_module_states` runs the modules' settings
   visitors in "remember" mode, copying the static values from `.data` into each setting's saved slot. Those slots are what the
   built-in Default profile restores and what a profile with missing fields uses as its fallback; the image's static `.data`
   only contains the live values, so without this step both paths would reset settings to zero. This is part of the inferred
   start-up order: the original's start-up lives in protector-virtualised code, so the call is not visible in `.text`.
4. State the frame and input code read: key table `java_natives_world_key_state_table_one_byte` (one byte per key), key map `java_natives_frame_profile_key_binds_key_profile`
   (profile binds, handler `on_profile_bind`, items are strings freed with `free`), key map `java_natives_frame_module_key_binds_key_module` (module binds,
   handler `on_module_bind`, several modules per key), profile list `init_profile_list` (`load_profiles` reads `~/.slinky/profiles`).
5. Chat commands `init_chat_commands`, GLFW entry points `init_glfw_entrypoints` (absent with LWJGL 2, not an error).
6. Saved settings `~/.slinky/settings` (**inferred**, mirror of `save_settings`): per-module state for global modules
   through `load_module_settings`, `profile_binds` through `bind_profile_to_key`.
7. `input_input_handler_hooked_into_game = on_input_event`: the input handler the window hook (`hook_game_window`, installed lazily every frame) calls.
8. Java side: `install_packet_channel` defines `SlinkyChannelHandler` (class bytes in the image) and registers `onPacket = on_packet`.
9. `install_class_hooks`: define `SlinkyHooks13`, register its 23 natives, add the `can_retransform_classes` capability, set the
   `ClassFileLoadHook` callback, enable the event, build the hook records for the detected mapping, retransform the 9
   classes, disable the event again. Errors are `12000 + jvmtiError`.

The hooks go last: from the first retransformation on, the game calls the natives.

## First frame (`hook_get_mouse_over`, hooks.cpp)

The first `getMouseOver` runs `gui_init` once, on the render thread: GL entry points
(`slinky_load_gl` + `wglGetProcAddress`), NanoVG context (`nvgCreateGL3`), window object `frame_render_target` with the two
embedded fonts (Inter Medium = slot 0, Inter Bold = slot 1). NanoVG flags are **inferred** (antialias + stencil strokes).

## The class hooks

`SlinkyHooks13` (class file in the image, 1196 bytes) declares 23 `public static native` methods; their bodies are
`hook_get_mouse_over .. hook_update_controller` (`kNatives` in `src/engine/jni/hook_tables.inc`, same order as the class file). The game classes are
not detoured: through JVMTI `RetransformClasses` the `ClassFileLoadHook` (`class_file_load_hook`) rewrites matching call and field
instructions into `invokestatic SlinkyHooks13.<native>`, so the receiver becomes the first `Object` argument.

What is rewritten is data: for each mapping (1.7.10 / 1.8.9 x obfuscated / MCP / SRG) the 9 classes
(Minecraft, EntityRenderer, RenderManager, RendererLivingEntity, RenderPlayer, NetHandlerPlayClient, WorldClient, Entity,
EntityLivingBase), per class the natives it may call and per method the instructions to replace
(`src/engine/jni/hook_tables.inc`, generated by `tools/gen_hooks.py` from an emulated run of the original installer,
`tools/dump_hooks.py`). 16 method patterns, 23 sites per mapping.

Rewritten call sites, by native: `getMouseOver` (EntityRenderer.getMouseOver in updateCameraAndRender),
`updateDisplay` (Display.update), `clickMouse`/`rightClickMouse`/`updateEntities`/`updateController` (inside Minecraft.runTick
and the input tick), `postWorldClear_1_7/_1_8` (glClear / GlStateManager.clear), `orientCamera`, `updateActiveRenderInfo`,
`isDisplayActivePreRender`, `setRenderX/Y` (RenderManager fields), `canRenderName` (any method of RendererLivingEntity),
`renderPlayer`, `unloadChunk`, `setBlock_1_7/_1_8`, `invalidateChunk`, `shouldClipLedge` (Entity.isSneaking), `moveFlying`,
`sendPacket_1_7/_1_8`.

**Verification:** `tools/emu_install.py` loads the built `slinky.dll`, runs `install_class_hooks` under Unicorn for all six mappings and
compares the records it builds with the original's: identical (9 classes, 16 patterns, 23 sites, same 23 natives).

## Packets

`SlinkyChannelHandler` (netty `ChannelDuplexHandler`, class bytes in the image) is put in the connection's pipeline as
`slinky_handler` (`register_channel_class`, every frame). It queues packets and calls `onPacket(Object packet, boolean outbound,
long time, int id)` = `on_packet`: under the lock `frame_spinlock`, and unless shutting down (`java_natives_frame_client_shutting_down_natives_check`) it dispatches module
event 2 (inbound) or 3 (outbound) and returns the handlers' verdict.

## Shutdown

`hook_update_display` (the `updateDisplay` hook) unloads when the GUI's confirmation was given: saves settings, notifies and disables the
modules, removes the netty handler, `set_java_hooks(env, 0)` (hooks off), `restore_game_window` (window hook off), then `start_unload_thread` starts a
thread that waits one second, runs `slinky_teardown` and calls `FreeLibraryAndExitThread`.
`slinky_teardown` (**inferred**) drops the netty class and the cached class references and frees the manager, both key maps,
the profile list, the chat command table, the profile name, the mesh cache, the key table and the ID table.
GL objects (NanoVG context, framebuffer) are not destroyed: that needs the render thread's context.

## Not verified / open

* The whole start-up order and the object sizes (`KeyMap` 0x30, key table 0x100, profile list 0x20) are inferred from their readers;
  nothing has run inside a game yet.
* `empty_placeholder` (decrypts a short string, only reached through `reset_key_state`, which nothing calls): empty placeholder.
* The NanoVG creation flags, JVMTI version (1.2) and the settings loader are inferred.
* Linux: no platform layer yet (JVMTI agent entry, input hook, file paths, GL loader).
