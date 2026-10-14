# Packages and the manifest

## The package layout in this repository

Each mod is a folder at the repository root whose name is the mod's ID:

```text
my-mod/
  manifest.json           the package manifest (below)
  README.md               what the mod does, for players
  LICENSE                 the licence the mod is distributed under
  src/
    mod.c                 the single compiled translation unit
    helper.h              optional local declarations
    feature_impl.h        optional implementation included by mod.c
  assets/                 optional licensed original artwork (.png, .glb), flat
    ASSETS.md             required with assets: author and licence of each file
  generate_art.py         optional pure original-art generator
  tools/                  optional reviewed Python setup modules
  tests/                  optional tests
```

- The packager compiles **only** `src/mod.c` (or a root-level `mod.c`). Include other `.c` files or
  implementation headers from it, as [dragon](../../dragon/src/mod.c) does. Keep sources flat in `src/`.
- `assets/` is flat: lowercase names (`a-z`, `0-9`, `_`, `-`), `.png` and `.glb` only, at most 4 MiB
  per file and 16 MiB per mod, each listed in `assets/ASSETS.md`. See [Art, layouts and licences](art.md).
- Never commit compiled files (`.o`, `.elf`, libraries), archives, generated images, game files,
  saves or keys. The catalogue build compiles the ELF and packages the ZIP.

The built package (a ZIP the port installs) contains `manifest.json`, `README.md`, `LICENSE`,
`mod.elf`, the sources under `src/`, your `assets/` and any setup `tools/`.

## The manifest

A minimal guest manifest:

```json
{
  "format_version": 1,
  "id": "my-mod",
  "name": "My mod",
  "version": "0.1.0",
  "game_id": "wwhd",
  "minimum_manager_version": "1.5.0",
  "kind": "guest",
  "guest": {"api_version": 1, "elf": "mod.elf"},
  "author": "Your name",
  "description": "One sentence describing the mod.",
  "setup": [
    {"id": "build", "type": "build_guest_mod", "title": "Build My mod for this installation"}
  ]
}
```

### Package fields

| Field | Meaning |
| --- | --- |
| `format_version` | `1` |
| `id` | lowercase ID: a letter or digit, then letters, digits, `_` or `-`, at most 64 characters; the folder name |
| `name`, `version` | display name; three-part version (`major.minor.patch`). Raise it for every update players should get |
| `game_id` | `"wwhd"`: this game, USA and EU. Give it with `"minimum_manager_version": "1.5.0"` |
| `minimum_manager_version` | at least `"1.5.0"` for catalogue packages |
| `kind` | `"guest"` for code mods (the only kind this catalogue packages) |
| `guest` | `api_version` `1`, `elf` `"mod.elf"`, optional `heap_size` (bytes; default 256 KiB, at most 8 MiB) |
| `author`, `description` | shown in the Mods tab; `author` must match the catalogue entry's `authors` |
| `experimental` | `true` for an untested mod, or one that can break the game or a save: hidden unless the player shows experimental mods; enabling it backs up the saves first. Must match the catalogue entry |
| `options` | typed options the player can change: `{"id", "type", "name", "default"}` with `type` `bool`, `number` (`min`, `max`, optional `step`), `string` or `enum` (`choices`). Read them with `wwhd_config_*` |
| `dependencies` | `[{"id": "story-notebook", "minimum_version": "0.6.0"}]`: packages that must be installed and on. Must match the catalogue entry's `requires` |
| `conflicts` | IDs of packages that can't be used together with this one |
| `setup` | the steps the port runs when installing: `build_guest_mod` (always, last), and for mods that need game data `game_path` and `run_tool` (see [Creating a mod](../creating-a-mod.md#mods-that-need-game-data)) |

### Story fields

These need the story system (port 0.2.15). They are explained with examples in [Story mods](story.md).

| Field | Meaning |
| --- | --- |
| `letters` | up to 8 letters the port sends while the mod is on; each `{"id", "sender", "text", "encloses"}` |
| `charts` | up to 8 charts: `{"id", "name", "description", "place": {"x", "z"}}` |
| `quests_format` | `1`, required with `quests` |
| `quests` | up to 8 quests for the story notebook: `id`, `title`, `journal`, `start`, `letter`, `reward`, `steps` |
| `npc_catalogue` | the NPC catalogue version your quests need (currently `3`) |
| `notebook` | only the Story Notebook package: the notebook's look (`icon`, `icon_open`, `model`, `layout`) |

A manifest with `quests` or `charts` depends on the Story Notebook package even if it doesn't say
so; declare the dependency anyway, so the catalogue lists it.

Coming in a later port version: an `items` block for the mod's own Delivery Bag items, and a chart's
`item` (what its treasure chest holds). Don't use them yet.

## The catalogue entry

Every mod also has an entry in the root [`catalogue.json`](../../catalogue.json) `mods` list. The
packager builds exactly the IDs listed there. Copy an existing entry and set:

```json
{
  "id": "my-mod",
  "name": "My mod",
  "description": "One sentence describing the mod.",
  "authors": ["Your name"],
  "version": "0.1.0",
  "kind": "guest",
  "licences": ["MPL-2.0"],
  "builds": ["USA", "EU"],
  "requires": [],
  "setup": [{"id": "build", "type": "build_guest_mod", "title": "Build My mod for this installation"}],
  "port_versions": {"minimum": "0.2.15"}
}
```

- `id`, `kind`, `version`, `setup` and `experimental` must be the same as in the manifest;
  `authors` is the manifest's `author` as a list; `requires` lists the manifest's dependency IDs.
- `port_versions.minimum`: the first port version with everything your mod uses (see the
  [version table](README.md#port-versions)).
- `art_generator`: `"generate_art.py"` when the mod generates its artwork (see [Art](art.md)).
- Never add `downloads`, URLs, sizes or hashes, and never edit `index.json`: the catalogue build
  writes them.

Next: [The C API](c-api.md).
