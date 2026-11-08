<p align="center">
  <img src="assets/ss.png" alt="Open Slinky Banner" width="350" />
  <br>
  <font size="6"><b>Open Slinky</b></font>
  <br>
  <font size="3">by Skid Solutions</font>
  <br>
  <font size="2">credits to cat;
</p>

---

<p align="center">
  <a href="https://discord.gg/AEwatvnvSf">https://discord.gg/AEwatvnvSf</a>
</p>

<p align="center">
  <img src="assets/jak.jpg" alt="java hooks" height="150" />
  <img src="assets/hack.jpg" alt="unload" height="150" />
  <img src="assets/deobf2.png" alt="symbol recovery" height="150" />
  <img src="assets/deobf.jpg" alt="readable names" height="150" />
</p>

---

## About

Open Slinky is a readable C++ reconstruction of the Slinky client for Minecraft
1.8.9. The client is injected into the game process as a native DLL; it hooks the
game at the Java bytecode level through JVMTI, dispatches events to the module
system, and renders its interface through NanoVG.

This edition is cleaned up for development:

- address-based file and function names have been replaced with descriptive
  ones; the original symbols are recorded in `docs/symbol-map.json` (functions)
  and `docs/global-symbol-map.json` (`data_<rva>` / `rdata_<rva>` globals and
  the `*_part` event handlers), and inferred names are listed in
  `docs/renames.md`;
- assembly used only for byte-exact code generation has been removed; the
  remaining `asm` tokens are symbol aliases, not instructions;
- the client's data tables remain in `src/data` because the code links against
  them (fonts, strings, module descriptors).

---

## How to build

### Prerequisites

- **LLVM** — clang-cl, lld-link, llvm-rc and llvm-mt on `PATH`
- **CMake** 3.20 or newer
- **Ninja**
- **Windows x64**
- **Java Development Kit 8** — runtime for Minecraft 1.8.9; not required to
  compile the DLL

### 1. Configure

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-windows-clang-cl.cmake
```

If Ninja is not on `PATH`, add
`-DCMAKE_MAKE_PROGRAM=C:/path/to/ninja.exe`.

### 2. Build

```powershell
cmake --build build
```

Output:

- `build/slinky.dll`

### Running

Inject `slinky.dll` into the running Minecraft 1.8.9 process with
`LoadLibrary`. Start-up runs on its own thread; the client interface is opened
with its GUI key. The last interface tab unloads the client: it saves the
settings, removes the Java hooks and frees the library.

---

## Project layout

```
src/engine/         hooks, JNI/JVMTI, GUI, module manager, math, shared services
src/logic/          per-module behaviour (.cpp) and its descriptor callbacks / settings page (.h)
src/platform/       Windows layer, GL loader, NanoVG integration
src/data/           data tables (fonts, strings, descriptors) - required to link
include/slinky/     public headers
docs/               design notes and the symbol map
third_party/        NanoVG and GL headers
```

## Notes

- Start-up, teardown, the window hook and the unload thread were virtualised in
  the original image and are reconstructed in `src/engine/protected/`.
- `docs/symbol-map.json` maps every original `fn_<rva>`, `logic_<rva>`,
  `cb_<rva>`, `reg_<rva>` and `aux_<rva>` symbol to its readable name.
- Settings and profiles are stored in `~/.slinky` (`settings`, `profiles/`).
- `src/data/*.s` and `src/platform/win/chkstk.s` are data/compiler-support
  sections, not client code, this is 1 to 1 logic but may have some issues that we didnt find.

## Credits

- Original client: Slinky
- Reconstruction: Claude (VMProtect devirtualization), DeepSeek (decryption),
  GLM and Claude (symbol recovery)

## Legal & disclaimers

- **Affiliation:** This is an independent development. It is **NOT AN OFFICIAL
  SLINKY PRODUCT**, is not approved by or associated with the original authors,
  and is not affiliated with Mojang or Zelix Pty Ltd.
- **Assets & code:** No un-obfuscated original source code is distributed
  directly within this repository. The `src/data` tables are the client's own
  data required for linking; all third-party assets and code belong to their
  respective owners.
- **Purpose:** research and preservation. Use at your own risk.
