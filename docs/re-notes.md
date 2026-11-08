# Reverse-engineering notes

Carried over from `slinkyfullsrc/SRC.md`. None of this is needed to build the
module sources.

## `.slinky0` — engine / module manager

`slinky2.decrypted/slinky0.bin` is 14 255 293 bytes (`.slinky0` of the client
image). Region map (verified):

| range | content |
|---|---|
| `0x000000–0x77FFFF` | native code + data (MSVC CRT/RTTI strings at `0x2d000–0x3c000`, engine code; clean x86-64, ~30 % printable) |
| `0x780000–0x8BFFFF` | high-entropy (7.99) blob — encrypted/compressed, not consumed by visible code |
| `0x8C0000–0x9FFFFF` | sorted `(u32 rva, u16 size)` table, 213 035 records, covers `0x36300E–0x867551`, sizes multiple of 8, dense (≈50 % of the region), deltas 2–16 bytes, overlap → looks like a per-site patch/relocation or checksum map for the code region |
| `0xA00000–0xAA0000` | table A, XOR `00 10 00 62 75 86`, 109 225 × `(u32 off, u16 size)`; offsets `0x223–0x27FFB4` fit *inside* the tail block; sizes ∈ {0,8,24,32,…,120} |
| `0xAA0000–0xAB0000` | table B, XOR `83 A7 00 10 00 53`, same 6-byte record shape |
| `0xAB0000–0xD80000` | `tail_high.bin` — 2 949 120 bytes, entropy 7.98, no standard container/PE; table A indexes into it |
| `0xD80000–0xD984BD` | `tail_code.bin` (~100 KB), mixed code/data |

Module-manager data model (recovered from the client image, used by the
reconstructed sources):

* Module descriptor in `.data` (`.rdata` string pointers + a factory/log
  function at `+0x58`). `modules_real.json` lists all 67 (61 with a GUI cb).
* Per-module settings block in `.data`, variable stride, begins with three
  function pointers `{gui_cb, register_settings, aux}` followed by the widget
  cfg objects (`CfgBool/CfgSlider/CfgRange/CfgEnum/CfgColor/...`).
* `R_*` registration calls are made from the client into the engine manager at
  DLL init (one `reg_<addr>` function per module), the `W_*` calls render the
  settings pane.

The module-side functions that read the settings and implement each feature
are in `src/logic/` (see `docs/logic.md`); they were extracted 1:1 from `.text`
by `tools/extract_logic.py`.

The manager itself is in client `.text`, not `.slinky0`: constructor `0x28000`,
dispatch `0x285d0`, descriptor table `0x14f810`.  It groups modules by category
and by the event callback slots at `descriptor+0x48` … `+0x1F8`, and dispatches
the enabled callbacks.  See `docs/engine.md` for the full picture, including
the six engine-only modules.  `.slinky0` has **no static RIP-relative
references** into the client's `.rdata`/`.data`/`.text` (checked: 0 refs), so
it receives whatever it needs through runtime-installed pointers.

### tail_high.bin

`tail_high.bin` is still packed. Findings:

* Not a PE, not ZIP/7z/RAR/gzip/zlib/LZMA (standard headers checked).
* Entropy 7.978; byte histogram ≈ uniform; no repeated 24-byte blocks.
* Table A (109 225 records) and the sorted table (213 035 records) both use the
  same `(u32 offset, u16 size)` record form, sizes are multiples of 8, and both
  point *into* their subject buffers with heavy overlap — this is a
  *site map* (patch/relocation/checksum), not a container directory.
* The runtime copy of `.slinky0` captured in `slinky.dll` is byte-identical to
  the static stream up to `0xC4B658` and zero beyond, i.e. that dump was taken
  before the engine performed the tail transformation; it gives no plaintext.
* Therefore decryption requires either (a) implementing the protector's
  per-site transform from the `0x8C0000` table, or (b) a live Wine trace of
  the first execution that touches `slinky0+0xAB0000` (same breakpoint
  technique that recovered the `.slinky2` LZMA streams).

Decrypting `tail_high.bin` is **not required** for anything in this tree: the
module callbacks/registrations are plain `.text`, and the registry data model
is plain `.data`. It is required only for the remainder of the engine internals.
