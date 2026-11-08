# Function renames (this pass)

Inferred from behaviour. `hitboxes_apply_hitbox_expansion_original` is the original engine copy; the descriptor uses the correctly named `hitboxes_on_reach_query` logic function. `block_esp_queue_loaded_chunks` was verified against its body (it queues the loaded chunks for scanning).

| old | new |
|---|---|
| `packet_entity_lookup` | `no_item_release_on_send_packet` |
| `activate_hold` | `hitboxes_apply_hitbox_expansion_original` |
| `compare_height_desc` | `compare_overlay_entries_by_height_desc` |
| `take_pending_bool` | `take_and_clear_pending_flag` |
| `rotation_cache_toggle` | `free_look_on_toggle` |
| `store_cached_rotation` | `free_look_save_rotation` |
| `enchantment_flags_item_stack` | `get_weapon_enchant_flags` |
| `weapons_part` | `is_held_item_accepted_weapon` |
| `click_with_helper` | `is_holding_accepted_weapon` |
| `close_screen_and_release` | `refill_disable_and_close_screen` |
| `find_hotbar_cell` | `refill_pick_inventory_cell` |
| `remove_id_key` | `friend_set_remove_by_key` |
| `clear_id_flag` | `friend_set_clear_dirty` |
| `compare_hurt_time` | `compare_by_hurt_time_desc` |
| `clear_list_set` | `free_block_search_section_lists` |
| `text_filter_upper` | `search_box_append_char` |
| `sort_float_entries` | `block_esp_queue_loaded_chunks` |
| `clear_mesh` | `rebuild_block_esp_mesh` |
| `free_allocator_pages` | `free_block_search_cache` |
| `pending_action_flag` | `timer_range_is_delay_pending` |
| `item_speed_by_type` | `no_slow_item_use_speed` |
| `zero_motion_tick` | `anti_debuff_tick` |

## Context struct (`Ctx`) fields

Every module handler receives a `Ctx *c`. Fields were previously `a`, `b`, `c18`, ...:

| old | new | meaning |
|---|---|---|
| `a` | `jvm` | JNI/JVM handle, first argument of every `java_*` call |
| `p08` | `module_manager` | target of `queue_module_toggle` |
| `p10` | `profile_list` | |
| `c18` | `minecraft` | Minecraft instance |
| `p20` | `world` | client world |
| `b` | `player` | local player entity |
| `p30` | `key_state_table` | |
| `p38` | `target` | current combat target record |
| `f40` | `in_game` | non-zero while a world is loaded |

## Event handler names

Most handlers are `<module>_on_<event>`, where the event is the descriptor slot the handler is registered in; handlers whose purpose is clearer keep a semantic name (for example `velocity_mode_3_delayed`, `notifications_draw_slide_boxes`):
`+0x48 tick`, `+0x58 frame`, `+0x68 packet_received`, `+0x78 packet_sent`, `+0x88 input`,
`+0x98 toggle`, `+0xa8 render_world`, `+0xb8 render_entity_overlay`, `+0xc8 reach_query`,
`+0xd8 hud_tick`, `+0xe8 hud_render`, `+0x118 rotation_update`, `+0x128 attack`,
`+0x138 attack_post`, `+0x158 player_update`, `+0x168 late_tick`, `+0x188 world_change`,
`+0x198 movement_input`, `+0x1a8 move_flying`, `+0x1b8 game_packet_sent`,
`+0x1d8 inventory_tick`, `+0x1e8 shutdown`. These slot meanings are inferred from the
handlers' behaviour, not from the original binary.

## Locals, parameters and struct fields

Renamed with a scope-aware AST pass (every edit is validated against the compiler and the generated object code):

- `Jvm *j`, `void *a` (first parameter) -> `jvm`; `Ctx *c` -> `ctx`; `*Hook *h` -> `hook`
- JNI accessor object parameters take the class from the accessor body, e.g. `a1` -> `game_settings`, `key_binding`, `entity_player`; setter value parameters are `value`
- locals initialised from an accessor are named after it (`mc` -> `minecraft`, `it` -> `stack`, ...)
- NanoVG context `ig` -> `nvg`, GUI state block `g` -> `gui`, `Rot r` -> `rotation`, register-function argument `m` -> `module`
- duplicate `HCtx` structs use the same field names as `Ctx` (`jvm`, `module_manager`, `minecraft`, `key_state_table`, `target`, `in_game`, ...)
- event structs: `f24` -> `verdict`, `p10` -> `packet`

## GUI state block (`include/slinky/gui_state.h`)

All raw `gui + 0x...` / `gui[0x...]` accesses are now named members of `struct GuiState`
(`toast_text`, `tabs`, `selected_tab`, `input_mode`, `edit_text`, `popup_kind`, `popup_rgb`, `popup_hsl`,
`popup_list_*`, `mouse_scene`, `scroll`, `row_height`, `click_capture`, ...). The layout-row stack at the start of the
block is `GuiState::rows[5]` (`GuiLayoutRow`, 0x78 bytes each). Offsets are checked with `static_assert`s; overlapping
uses (a text buffer that doubles as a key-bind callback, the popup payload) are unions. Field names are inferred
from how the GUI code uses them.

## `__N` disambiguators

Duplicate constants, strings and tables no longer carry `__N`: a symbol that is used from one file gets
`_in_<file>` (for example `row_height_20_in_gui_controls`), module state that already starts with the module name gets an
ordinal (`aim_assist_rng_states_second`), and symbols only referenced from the data tables get `_copy`.

## Verification note

The only generated-code difference from the original tree is in `gui_layout.cpp`, where the layout-row index is now
written `&gui->rows[n - 1]`; the compiler emits the same address computation with base and index registers swapped.

## Widgets, padding and parameter fixes

- Settings widgets: the `W1`/`W2`/`W3`/`Wid` structs became `DropdownSetting`, `ColorSetting`, `ListSetting`,
  `ToggleSetting`, `SliderSetting` and `RangeSetting`, with fields such as `label`, `format`, `step`, `scale`, `min`,
  `max`, `low`, `high`, `names`, `optional`, `popup_title`, `get_candidates`. `ListSet` / `ListOps` gained `add` and
  `remove`.
- Misnamed functions: `draw_keybind_control` -> `draw_dropdown_row`, `draw_checkbox` -> `draw_color_swatch_row`,
  `draw_colored_checkbox` -> `draw_list_editor_row`, `draw_label` -> `draw_toggle_row`, `draw_slider_label` ->
  `draw_slider_row`, `draw_slider_with_label` -> `draw_range_slider_row`, `draw_label_va` -> `draw_section_label`,
  `get_net_handler` -> `get_game_settings` (it returns `Minecraft.gameSettings`).
- Module GUI callbacks take `(GuiState *gui, window)` and the `widget_*` functions take `(window, gui, setting)`; an earlier
  pass had called the first argument `jvm`, which was wrong for these functions and is fixed.
- Padding fields are named by their byte offset (`padding_08`, `padding_11`, ...), computed from clang's record layouts.
  Offset-named fields with a known role got names (`on_shutdown`, `register_settings`, `fill_fraction`,
  `key_state_table`, `reach`, ...); the remaining zeroed filler bytes are `reserved_NN`.
- `hook_*` natives: `(env, native_class, receiver, ...)`; `java_jni_is_instance_of_*` takes `candidate`;
  integer error-code locals are `error_code`.

## Build fix and key-binding accessors

- `nvg_names.cpp` failed to compile after the parameter pass (the parameter was renamed to `nvg` but the macro-free bodies
  still used `c`); fixed. The two NanoVG-dependent files were not part of my per-file compile checks, so the full CMake
  cross-build is the reference check; it builds `slinky.dll` cleanly.
- Key-binding accessors are named by the `GameSettings.keyBindings` index they read:
  `java_get_game_settings_attack_key` (0), `_use_item_key` (1), `_forward_key` (2), `_left_key` (3), `_back_key` (4),
  `_right_key` (5), `_sneak_key` (7), `_key_8_or_9`, `_key_17_or_8` (mapping-dependent).
- Other numbered accessors: `java_get_minecraft_object_mouse_over`, `java_get_minecraft_player_controller`,
  `java_get_entity_player_inventory`, `java_get_moving_object_position_type_ordinal` / `_entity_hit`,
  `java_increment_key_binding_press_time`.
- `timer_range_last_forward_input` / `timer_range_last_strafe_input` were swapped in an earlier pass; fixed (they come
  from the forward/back and left/right keys respectively).
- `CombatTarget` (engine.h) replaces raw `ctx->target + 0x18` reads with `->valid`.

## JNI calls, numbered names, constants, padding

- Raw JNI function-table calls (`(*(void ***)env)[0xb0 / 8]` and friends) are now named calls through the existing
  `JNIEnv` table: `(*env)->DeleteGlobalRef`, `IsSameObject`, `NewGlobalRef`, `NewStringUTF`, `GetObjectArrayElement`,
  `GetStringLength`, `GetStringCritical`, `ReleaseStringCritical`. `((void **)ctx->jvm)[1]` is `((Jvm *)ctx->jvm)->env`.
- Functions that ended in `_2` / `_3` have descriptive names (`friends_on_tick`, `friends_on_toggle`,
  `bridge_assist_on_late_tick`, `piercing_decide_which_entity`, `java_get_world_chunk_provider_client`, ...). The numbers
  that remain are meaningful (`hook_*_1_7` / `_1_8` Minecraft versions, `player_esp_render_mode_0/1`, key-binding array
  indices, `java_call_world_get_block_state/500` method ids).
- Constants used as arguments of a recognised call are named by role (`font_size_16_in_gui_popups`,
  `corner_radius_10_in_gui_text`, `colour_level_0_866667_in_gui_text`, `walk_period_0_5_in_clutch`,
  `random_range_bound_0_05_in_clutch`); constants used in arithmetic or comparisons by operator
  (`threshold_`, `factor_`, `divisor_`, `offset_`); pi and the 180 / 360 degree constants are `pi_constant`,
  `half_turn_degrees`, `full_turn_degrees`. Constants whose use is not one of these keep their value in the name.
- Struct filler bytes are `padding_NN` (offset in hex), not `reserved_NN`: they are never read.

## Minecraft field / method identifiers, string names, constants

The Minecraft JNI identifiers (`JIds` members) are now named after what they are in the game
(`Minecraft_theWorld`, `Minecraft_thePlayer`, `Minecraft_objectMouseOver`, `Entity_boundingBox`, `World_chunkProvider`,
`MovingObjectPosition_hitVec`, `InventoryPlayer_itemStack`, `RenderManager_textRenderer`, ...), and their accessors
follow (`java_get_minecraft_render_item`, `java_get_entity_riding_entity`, ...). Members whose game name I could not
establish from the code are called `<Class>_unidentified_<offset>`; method ids are `unidentified_<offset>`.

**Compatibility:** only C identifiers changed. The class / field / method *name strings* that the client resolves at
run time (the obfuscated or mapped names used for the JVM lookups, including any mapping a particular launcher needs)
live in `src/data/rdata.s` and are untouched; the data section hashes identically before and after. Short obfuscated
name strings are labelled `mc_obf_name_<text>`, JNI signature strings `jni_signature_<text>`, printf formats
`fmt_<spec>` and X.509 OID strings `oid_<digits>`.

Also: `fn_<address>` / `logic_<address>` references in the docs now show the current function names;
`java_get_static_field_c80` is `java_get_c16_enum_state_open_inventory`; remaining one- and two-letter locals of
struct-pointer type are named after the struct (`entry`, `target`, `visitor`, `window`, ...); constants used as plain
literals are named by number word (`one`, `half`, `minus_one`, ...) or by role (`range_low_*`).

## JNI members named from the client's own mapping tables

Every `JIds` member (fields and methods of the Minecraft classes the client resolves through JNI) is now named after the
MCP name that the client's own data tables carry next to the obfuscated name for each mapping (`getBlockState`,
`rayTrace`, `sideHit`, `renderGlobal`, `netManager`, `updateCameraAndRender`, ...), so the names are the ones the data
itself uses and are verified against it. Earlier guesses that the tables contradicted were corrected
(`World_playerEntities` / `World_loadedEntityList` / `World_loadedTileEntityList`, `TileEntity_blockType`,
`NetworkPlayerInfo_displayName`, `GuiContainer_theSlot`, `RenderManager_textRenderer`, `InventoryPlayer_itemStack`).
The string slots that hold method names and signatures for the render hooks are `RendererLivingEntity_*MethodName` /
`*MethodSignature`. No `unidentified_*` names remain. Only identifiers changed; the name strings are unchanged.

## Constants

All remaining `k_f_*` / `k_d_*` constants are named by purpose (`ground_friction_times_air_drag`,
`movement_input_friction`, `gui_wide_layout_max_width`, ...).
