# Changes in this build

## Root cause of "Slinky could not start" (`NoSuchMethodError: Minecraft.getMinecraft()`)
The original `slinky-srg.jar` / `slinky-official.jar` were NOT remapped: their bytecode still called
MCP names (`getMinecraft`, `gameSettings`, `rotationYaw` ...) and only a `mappings.txt` was added.
On any runtime that is not MCP-named (Forge = SRG, Vanilla/OptiFine/Lunar/Badlion = notch) the first
direct call crashed. `tools/verify_payload.py` finds 303 (srg) / 340 (official) stale references in the
old jars and 0 in the new ones.

## What changed
| File | Change |
|---|---|
| `SlinkyNative/payload/slinky-srg.jar`, `slinky-official.jar`, `slinky-named.jar` | Fully remapped payloads (from the BLC-Slinky work): srg for Forge, notch for Vanilla/OptiFine/Lunar/Badlion. |
| `SlinkyNative/src/PayloadLoader.cpp` | `addURL` is best-effort (JDK 9+ loaders are not URLClassLoaders); classes are defined directly in multiple dependency-aware passes; **new:** `addURL` is only called on real `URLClassLoader`s (calling it on anything else is undefined behaviour in JNI). |
| `SlinkyNative/src/MinecraftDetector.cpp` | Rewritten: checks both `net/minecraft/client/Minecraft` and `ave`, finds the singleton field by type instead of one hard-coded name, waits until the game object exists (Lunar/Badlion load the class early), frees JNI refs while polling, logs loader class, and gives a diagnostic log for unknown naming schemes. |
| `Slinky/src/Main.cpp` | Injects into the JVM that has LWJGL loaded (prefers titles with Minecraft/Lunar/Badlion) instead of the first Java window on the machine. |
| `Slinky/src/Injector.cpp` | Startup monitor timeout 30 s -> 150 s (Bootstrap can legitimately wait 2 min). |
| `build.ps1`, `tools/verify_payload.py` | One-command Windows build; static payload checker. |

## Not changed
`SlinkyClient/` (Java sources) is identical to upstream. Rebuilding it with Gradle only produces the *named* jar.
