# GameCube minimap

Adds a GameCube-style corner map to the TV during Great Sea exploration and in supported dungeon rooms. Version 0.3.0 adds current-room dungeon maps.

Install through **Mods → Browse**, enable code mods when offered, select your own GameCube Wind Waker RVZ, ISO or full extracted game folder, run **Prepare local maps**, build, then enable and restart. Re-run preparation when updating from an earlier version. USA, European and Japanese GameCube sources are accepted; the running HD installation may be USA or European. The selected source must contain the full game’s map files.

Your game files stay on your machine. Preparation writes derived maps only into the mod’s local data storage; no game maps are included in the distributed package. RVZ files are read directly without ISO conversion. Zstandard-compressed RVZ files need the updated port’s private preparation interpreter. An older installation can refresh its preparation tools with the setup repair option.

Dungeon maps follow the GameCube **current-room corner map**: entering a room or changing floors selects that room’s image, with Link’s position and facing. Owning the dungeon map does not reveal neighboring room silhouettes in this panel; that is the pause-screen map’s behavior. The compass enables unopened-chest markers. Doors are marked separately. The panel hides during events, transitions, pauses and TV menus.

Coverage depends on the original room files. It includes Dragon Roost Cavern, Forbidden Woods, Tower of the Gods, Earth Temple, Wind Temple, their supported miniboss rooms, and Forsaken Fortress areas. Ganon’s Tower, main boss arenas and ordinary secret caves without complete GameCube corner maps stay hidden. Preparation also rejects missing or invalid map bounds.

Mod dungeons that supply a map can show their current room using the port’s existing room artwork, including compass markers. This requires the updated port; older hosts omit mod-dungeon maps while retaining prepared GameCube room maps. Mod-room artwork is the same in both style settings.

Left and bottom margins, panel size, opacity and original/vector map style remain configurable. **Show dungeon maps** turns room maps off while keeping sea maps enabled. There is no separate boat marker. Enabling, disabling and option changes take effect after restarting.

## Changes

- **0.3.0:** adds current-room dungeon maps, floor selection, live compass markers, optional mod-dungeon maps and a dungeon-map toggle.
- **0.2.1:** hides the panel while a pause menu or another menu is shown on the TV.
