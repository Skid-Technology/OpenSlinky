# Modules by category

Category = descriptor field `+0x10` (0 Combat, 1 Move, 2 Visual, 3 Block,
4 Utility), matching the GUI tab strings at `.rdata` `0x151cc8`. Paths are
relative to the repo root.  The first five sections are the 61 modules with a
GUI callback; the six engine-only modules are listed at the end.

### Combat (17)

| module | file | functions | description |
|---|---|---|---|
| Aim Assist | [aim_assist.h](../src/logic/combat/aim_assist.h) | cb, reg, aux | Pulls your crosshair towards other players. |
| Auto Block | [auto_block.h](../src/logic/combat/auto_block.h) | cb, reg | Blocks your sword for you in combat. |
| Auto Clicker | [auto_clicker.h](../src/logic/combat/auto_clicker.h) | cb, reg, aux | Clicks for you when holding down left-click. |
| Auto Weapon | [auto_weapon.h](../src/logic/combat/auto_weapon.h) | cb, reg | Selects your weapon when aiming on players. |
| Backtrack | [backtrack.h](../src/logic/combat/backtrack.h) | cb, reg, aux | Uses lag to make players easier to hit. |
| Criticals | [criticals.h](../src/logic/combat/criticals.h) | cb, reg, aux | Increases chance of landing critical hits. |
| Hit Select | [hit_select.h](../src/logic/combat/hit_select.h) | cb, reg, aux | Perfectly times your hits. |
| Hitboxes | [hitboxes.h](../src/logic/combat/hitboxes.h) | cb, reg, aux | Expands hitboxes of other players. |
| Knockback Delay | [knockback_delay.h](../src/logic/combat/knockback_delay.h) | cb, reg, aux | Delays your knockback in certain scenarios. |
| Knockback Displace | [knockback_displace.h](../src/logic/combat/knockback_displace.h) | cb, reg, aux | Allows dealing knockback at a different angle. |
| Lag Range | [lag_range.h](../src/logic/combat/lag_range.h) | cb, reg, aux | Uses lag to stay out of target range. |
| Piercing | [piercing.h](../src/logic/combat/piercing.h) | cb, reg | Allows hitting entities through obstructions. |
| Reach | [reach.h](../src/logic/combat/reach.h) | cb, reg, aux | Increases your melee range. |
| Sprint Reset | [sprint_reset.h](../src/logic/combat/sprint_reset.h) | cb, reg, aux | Resets your sprint state to deal more knockback. |
| Timer Range | [timer_range.h](../src/logic/combat/timer_range.h) | cb, reg, aux | Abuses timer lag to teleport you. |
| Velocity | [velocity.h](../src/logic/combat/velocity.h) | cb, reg, aux | Reduces the amount of knockback you take. |
| Weapons | [weapons.h](../src/logic/combat/weapons.h) | cb, reg | Configure which items are considered weapons. |

### Move (7)

| module | file | functions | description |
|---|---|---|---|
| Fast Accel | [fast_accel.h](../src/logic/move/fast_accel.h) | cb, reg | Speeds up acceleration. |
| Inv Walk | [inv_walk.h](../src/logic/move/inv_walk.h) | cb, reg, aux | Allows moving in inventory. |
| Keep Sprint | [keep_sprint.h](../src/logic/move/keep_sprint.h) | cb, reg, aux | Keeps your sprint speed when attacking players. |
| No Slow | [no_slow.h](../src/logic/move/no_slow.h) | cb, reg | Removes slowdown while using items. |
| Safe Walk | [safe_walk.h](../src/logic/move/safe_walk.h) | cb, reg | Prevents you from walking off ledges. |
| Sprint | [sprint.h](../src/logic/move/sprint.h) | cb, reg | Automatically sprints for you. |
| Strafe | [strafe.h](../src/logic/move/strafe.h) | cb, reg | Allows faster strafing. |

### Visual (14)

| module | file | functions | description |
|---|---|---|---|
| Anti Debuff | [anti_debuff.h](../src/logic/visual/anti_debuff.h) | cb, reg | Removes negative visual effects. |
| Block ESP | [block_esp.h](../src/logic/visual/block_esp.h) | cb, reg | Highlights blocks. |
| Chams | [chams.h](../src/logic/visual/chams.h) | cb, reg | Renders players behind blocks. |
| Chest ESP | [chest_esp.h](../src/logic/visual/chest_esp.h) | cb, reg | Highlights chests. |
| Free Look | [free_look.h](../src/logic/visual/free_look.h) | cb, reg | Freely move the camera around. |
| GUI | [gui.h](../src/logic/visual/gui.h) | cb, reg | Graphical interface for the cheat. |
| Indicators | [indicators.h](../src/logic/visual/indicators.h) | cb, reg | Points to incoming projectiles. |
| Item ESP | [item_esp.h](../src/logic/visual/item_esp.h) | cb, reg | Highlights dropped items. |
| Mod Overlay | [mod_overlay.h](../src/logic/visual/mod_overlay.h) | cb, reg | Displays a list of all active mods. |
| Name Tags | [name_tags.h](../src/logic/visual/name_tags.h) | cb, reg | Makes player name tags easier to see. |
| Notifications | [notifications.h](../src/logic/visual/notifications.h) | cb, reg | Informs you when certain actions are performed. |
| Player ESP | [player_esp.h](../src/logic/visual/player_esp.h) | cb, reg, aux | Highlights other players. |
| Pointers | [pointers.h](../src/logic/visual/pointers.h) | cb, reg, aux | Points to other players. |
| Trajectories | [trajectories.h](../src/logic/visual/trajectories.h) | cb, reg | Predicts the trajectory of the item in your hand. |

### Block (7)

| module | file | functions | description |
|---|---|---|---|
| Auto Tool | [auto_tool.h](../src/logic/block/auto_tool.h) | cb, reg | Selects the best tool when digging. |
| Block In | [block_in.h](../src/logic/block/block_in.h) | cb, reg | Places blocks around you. |
| Bridge Assist | [bridge_assist.h](../src/logic/block/bridge_assist.h) | cb, reg | Sneaks and unsneaks for you while bridging. |
| Clutch | [clutch.h](../src/logic/block/clutch.h) | cb, reg | Clutches for you when falling. |
| Fast Mine | [fast_mine.h](../src/logic/block/fast_mine.h) | cb, reg, aux | Increases block mining speed. |
| Fast Place | [fast_place.h](../src/logic/block/fast_place.h) | cb, reg, aux | Allows placing blocks faster. |
| Ghost Hand | [ghost_hand.h](../src/logic/block/ghost_hand.h) | cb, reg | Allows interacting with blocks through entities. |

### Utility (16)

| module | file | functions | description |
|---|---|---|---|
| Anti Bot | [anti_bot.h](../src/logic/utility/anti_bot.h) | cb, reg | Ignores NPCs for specific modules. |
| Auto Pot | [auto_pot.h](../src/logic/utility/auto_pot.h) | cb, reg | Uses healing potions for you. |
| Auto Rod | [auto_rod.h](../src/logic/utility/auto_rod.h) | cb, reg | Throws your fishing rod for you. |
| Auto Soup | [auto_soup.h](../src/logic/utility/auto_soup.h) | cb, reg | Uses soups for you. |
| Blink | [blink.h](../src/logic/utility/blink.h) | cb, reg, aux | Simulates teleporting using lag. |
| Chest Stealer | [chest_stealer.h](../src/logic/utility/chest_stealer.h) | cb, reg | Automatically loots chests for you. |
| Debug | [debug.h](../src/logic/utility/debug.h) | cb, reg | Prints debug information in chat. |
| Fake Lag | [fake_lag.h](../src/logic/utility/fake_lag.h) | cb, reg, aux | Adds an artificial delay to your packets. |
| Friends | [friends.h](../src/logic/utility/friends.h) | cb, reg | Lets you mark other players as friendly. |
| Inv Manager | [inv_manager.h](../src/logic/utility/inv_manager.h) | cb, reg | Organises your inventory for you. |
| No Hit Delay | [no_hit_delay.h](../src/logic/utility/no_hit_delay.h) | cb, reg, aux | Removes 1.8 swing delay. |
| No Item Release | [no_item_release.h](../src/logic/utility/no_item_release.h) | cb, reg, aux | Hides item release from server. |
| No Use Delay | [no_use_delay.h](../src/logic/utility/no_use_delay.h) | cb, reg | Removes vanilla delay when consuming items. |
| Refill | [refill.h](../src/logic/utility/refill.h) | cb, reg | Automatically moves items into your hotbar. |
| Teams | [teams.h](../src/logic/utility/teams.h) | cb, reg | Ignores teammates for specific modules. |
| Timer | [timer.h](../src/logic/utility/timer.h) | cb, reg, aux | Modifies the speed the game runs at. |

### Engine-only (6)

These descriptors exist in the image but have no GUI callback and no settings
pane. They are kept in `manifest.json` so the module table is exactly the 67
descriptors from the image.

| module | category | file | description |
|---|---|---|---|
| No Jump Delay | move | [no_jump_delay.h](../src/logic/move/no_jump_delay.h) | Removes cooldown when holding jump key. |
| Ping Fix | utility | [ping_fix.h](../src/logic/utility/ping_fix.h) | Shows your real ping while using lag mods. |
| No Hurt Cam | visual | [no_hurt_cam.h](../src/logic/visual/no_hurt_cam.h) | Removes camera shake when taking damage. |
| Instant Stop | move | [instant_stop.h](../src/logic/move/instant_stop.h) | Instantly stops moving when move keys are released. |
| Item Use Fix | utility | [item_use_fix.h](../src/logic/utility/item_use_fix.h) | Allows using items while breaking blocks. |
| Null Move | move | [null_move.h](../src/logic/move/null_move.h) | Makes it easier to switch move direction instantly. |
