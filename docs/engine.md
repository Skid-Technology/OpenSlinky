# Engine / module-manager notes

This is the result of looking for the remaining module logic in `.slinky0` and
in the client's own dispatcher.

## The module manager is in client `.text`

The manager is not in `.slinky0`.  The constructor is at `0x28000` in the
client `.text`:

- it reads the descriptor table at `0x14f810`,
- stores the module count `0x43` (67 descriptors),
- groups modules by category (`descriptor+0x10`),
- builds callback lists for the event slots at
  `descriptor +0x48, +0x58, …, +0x1F8`,
- writes `[descriptor+0x14] = 1` when a module is enabled.

The dispatcher at `0x285d0` walks the enabled modules, collects the callback
for the requested event slot, and calls it.  This is why the descriptor
callbacks are the primary module logic: a module without a callback at any of
those slots has no client-side handler for that event.

## The six engine-only modules

The descriptors for these modules were checked through all event slots:

| module | client callback(s) | result |
|---|---|---|
| No Jump Delay | `0x35d50` (`+0x158`) | extracted |
| Ping Fix | `0x3e6d0` (`+0x68`), `0x3e700` (`+0x78`) | extracted |
| Instant Stop | `0x42650` (`+0x148`) | extracted |
| Item Use Fix | `0x42b80` (`+0x128`), `0x42be0` (`+0x138`) | extracted |
| Null Move | `0x46ac0` (`+0x148`) | extracted |
| No Hurt Cam | none | no client callback |

`No Hurt Cam` also has no RIP-relative reference anywhere in the client
`.text`, and its descriptor has no callback pointer in `+0x48 … +0x1F8`.
There is therefore no client-native implementation to copy.  Its effect must
be applied by an engine/Java-side hook (or by code in the still-packed
`.slinky0` tail); the decrypted clean engine region contains no module names,
no descriptor references and no module-manager code.

## `.slinky0`

`.slinky0` is a raw native runtime blob, not a PE.  The clean region contains
native code/data (a PE loader, CRT/RTTI data, networking/OpenGL support), but
none of the module names or the descriptor table.  The module table is passed
to it at runtime, and the module dispatch is done by the client manager above.
The remaining engine tail (`slinky0+0xAB0000 …`) is still packed, so nothing
can be copied from it statically without defeating that transform.
