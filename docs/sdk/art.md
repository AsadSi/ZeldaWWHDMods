# Art, layouts and licences

## The rule

Every image, model and text in a package is your own work, or work you have the rights to share.
Never game material: no extracted, traced or converted game textures, models, fonts or screenshots,
and nothing derived from them. Mods that need the game's look use the player's own game at run time
(the game's fonts, its menu pictures), and mods that need game data prepare it on the player's
machine with a reviewed setup tool (see [Creating a mod](../creating-a-mod.md#mods-that-need-game-data)).

There are two ways to bring original art:

1. **Generated art.** A pure Python module `generate_art.py` in the mod folder exports `generate()`,
   returning a dictionary of flat PNG filenames to PNG bytes. Declare it in the catalogue entry
   (`"art_generator": "generate_art.py"`). The catalogue build runs it and puts the pictures in the
   package's `assets/`. It may use only the Python standard library's parsers and maths (no files,
   network or other programs), and each picture is at most 1 MiB. Commit the generator, never its
   output. See [the minimap's generator](../../gc-minimap/generate_art.py).
2. **Licensed assets.** Painted pictures and 3D models that can't come from a generator are
   committed in the mod's `assets/` folder:
   - flat files, lowercase names (`a-z`, `0-9`, `_`, `-`), only `.png` and `.glb` (binary glTF 2.0);
   - at most 4 MiB per file and 16 MiB per mod;
   - `assets/ASSETS.md` lists every file with its **author** and its **licence type** (for example
     the package's licence, or CC BY 4.0). The author must hold the rights;
   - the maintainer opens every file before it is merged, and the packages refuse game archive
     signatures.

A small text file that belongs to the pictures (the Story Notebook's page layout, below) may sit in
`assets/` beside them; it is reviewed like source.

## Formats and sizes the port accepts

| Use | Format | Limits |
| --- | --- | --- |
| HUD images (`wwhd_hud_texture`) | PNG | at most 2048 × 2048 each; 32 images and 16 MiB of decoded pixels per mod |
| Quest portrait (`wwhd_quest_portrait`) | PNG in `assets/` | up to 256 × 256 |
| Item icon (`items[].icon`, port 0.3.0) | PNG in `assets/` | any size, scaled to 128 × 128; 128 × 128 with transparency works best. The port adds it to the game's item pictures on the player's machine |
| Story Notebook icon (`notebook.icon`, `notebook.icon_open`) | PNG | up to 1024 × 1024 and 4 MiB; 192 × 192 with transparency works well |
| Story Notebook model (`notebook.model`) | binary glTF 2.0 (`.glb`) | up to 8 MiB |
| Story Notebook page (`notebook.layout`) | layout description (JSON) and PNGs in its folder | JSON up to 1 MiB; up to 64 PNGs and 64 MiB of decoded pixels in that folder |

## The Story Notebook's look

Only the [Story Notebook package](../../story-notebook/) has a `notebook` block:

```json
"notebook": {
  "icon": "assets/notebook_icon.png",
  "icon_open": "assets/notebook_icon_open.png",
  "model": "assets/notebook_game.glb",
  "layout": "assets/notebook.json"
}
```

- `icon` is the notebook in the Delivery Bag; `icon_open` the notebook drawn open while its page
  opens and closes.
- `model` is the notebook's 3D model. The port turns it into the game's model format on the player's
  machine.
- `layout` is the notebook page: a layout description that names its pictures (by filename, in the
  same folder), panes, materials and animations. The port builds the page from it on the player's
  machine. The pane names are the contract with the port's notebook code, so a new look keeps the
  layout and changes its pictures: replace a PNG with your own picture of the same size and keep its
  name. The layout also names the game's own fonts and one game icon by their names; the port takes
  them from the player's game.

All of these are licensed assets: list them in `assets/ASSETS.md`.

Next: [Testing and submitting](testing.md).
