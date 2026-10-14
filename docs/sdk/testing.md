# Testing and submitting

## Test in the port

1. Build the ELF and stage the package folder as in [Setup](setup.md#a-first-build).
2. In the port: **Settings (F1) → Mods → Installed packages → Choose folder**, choose the staged
   folder, install it, confirm trust, complete the setup, enable the mod and restart.
3. Check what players will do: install, setup, enable, restart, disable and remove; the USA and
   European game; 30 and 60 fps; Metal and Vulkan; save files and save states; recovery from a
   failed setup step. With the mod off, the game must behave exactly as without it.
4. For a story mod: a save file before and after the Delivery Bag, the letter at a Rito postbox, the
   notebook's rows and **Show on chart**, each step, loading an older save (progress not saved is
   undone) and a Quest Log your mod never saw.

The log (and the package's details in the Mods tab) shows build errors, refused manifest fields
("Story quests: ...") and your `wwhd_log` lines. Write down what you could not test (a region, a
platform) in your pull request; don't commit game-derived screenshots or saves as evidence.

Mark an untested mod, or one that can break the game or a save, with `"experimental": true` in the
manifest and its catalogue entry.

## Run the repository's checks

The full package build needs a clean, committed checkout of this repository and a clean checkout of
the port at the commit pinned in [`sdk.json`](../../sdk.json) (see [Setup](setup.md#the-sdk-headers)).
Commit your work first, then:

```sh
SDK=../port
python3 tools/guard.py . --sdk "$SDK"
python3 tools/test_guard.py "$SDK"
python3 tools/test_package.py
python3 tools/package.py --sdk "$SDK" --out build/packages --clang clang --lld ld.lld \
  --channel devel --base-url https://github.com/ZeldaWWHDRecomp/ZeldaWWHDMods/releases/download
python3 tools/validate.py build/packages --root-index catalogue.json
python3 tools/test_validate.py build/packages
```

- `guard.py` checks the sources: no compiled files, archives, encoded blobs or game data; licensed
  assets in the right place with their `ASSETS.md`; setup tools and art generators within the
  allowed Python.
- `package.py` compiles every catalogue mod against the pinned SDK and builds the ZIPs and a local
  index.
- `validate.py` checks the packages against `catalogue.json` (IDs, versions, experimental flag,
  game ID, dependencies, the ELF).
- Run your mod's own tests too (for example `python3 -m unittest discover -s dragon/tests -p 'test_*.py'`).

On macOS, pass `--clang "$(brew --prefix llvm)/bin/clang"` and your lld's `ld.lld`. The local index
records the real HTTPS download URLs; to try a built ZIP in the port, install it with **Installed
packages → Choose file**.

## Submit

Read [CONTRIBUTING.md](../../CONTRIBUTING.md). In short:

1. Fork the repository, add your mod folder and its `catalogue.json` entry, and open a pull request
   into **devel**. Include the licence, your test notes and known limitations.
2. Submit sources only: no packages, compiled files, archives, generated images, game material,
   saves or keys. Don't edit `index.json`, `sdk.json`, `tools/` or the workflows.
3. CI runs the same checks with the pinned SDK, read-only and without secrets. The maintainer reviews
   every hook, service call, setup step and asset (see the [review checklist](../review-checklist.md)).
4. Reviewed mods are published to the devel catalogue for testing, and later promoted to **main**,
   which players load by default.

To update a mod, raise its `version` in the manifest and the catalogue entry.
