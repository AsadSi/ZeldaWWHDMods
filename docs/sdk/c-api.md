# The C API

Include `wwhd_guest.h` and the generated `wwhd/` headers from the pinned SDK. Everything below is
declared there; the header comments are the exact contract.

## Hooks and replacements

| Macro | Runs |
| --- | --- |
| `WWHD_HOOK(target, name, (args))` | before the game function, with its arguments |
| `WWHD_HOOK_RETURN(target, name, (args))` | after it, with the same arguments; the game's result is kept |
| `WWHD_REPLACE(target, ret, name, (args))` | instead of it |
| `WWHD_GAME_ORIGINAL(...)` | inside a replacement: the game's own code, below all mod hooks |

- Every game function can be hooked, including calls through function pointers.
- Hooks can't change the arguments or the result. To change them, replace the function and call the
  original yourself.
- Only one mod may replace a function; a second one is an error naming both mods. Any number of
  mods may hook one. Entry hooks run in load order, return hooks in reverse order.
- A hook on a logic function runs once per logic step, at any frame rate; a hook on a drawing
  function runs once per displayed frame. Use `wwhd_logic_dt()` for anything time-based, and don't
  count displayed frames as game steps. Keep frequent hooks small.
- The calling convention is the game's: arguments in r3–r10 / f1–f8, results in r3/r4/f1.

A typical gameplay hook, and the check that a save file is loaded:

```c
#include "wwhd_guest.h"
#include "wwhd/functions.h"
#include "wwhd/save.h"

WWHD_HOOK(WWHD_ADDR_daPy_Execute, my_step, (void* link)) {
    (void)link;
    u16 max_life = dComIfGs_player_status_a.max_life;
    if (max_life < 12 || max_life > 80) return;  /* no save file loaded */
    /* ... */
}
```

## Game functions, data and objects

The headers come from the public [HD decompilation](https://github.com/ZeldaWWHDDecomp/wwhd) and use
the USA game's addresses; the port maps them for European installs.

- `wwhd/functions.h`: every public verified function as a hook target, `WWHD_ADDR_<name>`.
  Ambiguous names have an address suffix.
- `wwhd/bindings.h`: callable declarations, `wwhd_<name>(...)`. Object pointers are `void*`.
  Functions returning a register pair use `wwhd_gpr_pair` with `WWHD_RESULT_R3` / `WWHD_RESULT_R4`.
- `wwhd/data.h`: named save/resource pointers, matrix stack and item tables.
- `wwhd/actor.h`, `link.h`, `camera.h`, `items.h`, `messages.h`, `save.h`: partial views of game
  objects with checked field offsets; unknown parts are reachable as bytes.
- Functions not in the public decomp can still be hooked by address with
  `WWHD_GAME_FUNC(address, ret, name, (args))`; game variables with `WWHD_GAME_DATA(address, type)`.

**European installs.** Write your mod against USA addresses; when it is built on an EU install, the
port translates hook targets, game calls, function pointers and `WWHD_GAME_DATA` references. Functions
that differ or don't exist in EU are refused with an error naming them. Don't cast integers to
pointers (that hard-codes a USA address), and only use object fields that are the same in both
versions.

## Host services

| Service | What it does |
| --- | --- |
| `wwhd_log`, `_int`, `_hex`, `_float` | log lines tagged with your mod's ID |
| `wwhd_config_int`, `_bool`, `_float`, `_string` | your mod's options (values from startup; the fallback on a missing option or wrong type) |
| `wwhd_malloc`, `wwhd_free` | per-mod heap, 16-byte aligned; size from `guest.heap_size`; null when full |
| `wwhd_input_read` | buttons, sticks and touch (read-only) |
| `wwhd_file_read`, `wwhd_file_write` | flat file names in your mod's own data folder, at most 1 MiB per call; write replaces the file; return bytes or -1 |
| `wwhd_logic_dt`, `wwhd_logic_step` | the length of the current logic step in seconds (60 fps modes included) and the step counter |
| `wwhd_setting_get`, `wwhd_setting_changed` | read-only port settings, below |
| `wwhd_hud_*` | drawing on screen, below |
| `wwhd_audio_*` | bounded PCM audio streams, below |
| `wwhd_mail_*`, `wwhd_chart_*`, `wwhd_quest_*` | letters, charts and quests: [Story mods](story.md) |
| `memcpy`, `memmove`, `memset` | as usual |

Your mod can also call any game function through `wwhd/bindings.h`.

## Port settings

`wwhd_setting_get(key, type, buffer, capacity)` returns the bytes written, or 0 for an unknown key,
wrong type or too small buffer (the buffer stays unchanged). Pass a null buffer and 0 to get the
size. Strings include the NUL; numbers are big-endian. Mods never change settings.
`wwhd_setting_changed(key)` returns a revision that changes when the value changes; read settings
once per logic step.

| Key | Type | Values |
| --- | --- | --- |
| `input.face_layout` | STRING | `position`, `labels`, `custom` |
| `input.controller_mode` | STRING | `gamepad`, `pro` |
| `game.language` | U32 | Wii U language code 0–11 in use (set at startup) |
| `game.build` | STRING | `USA`, `EU` |
| `display.drc_mode` | STRING | `window`, `pip`, `auto`, `off`, `gamepad` |
| `display.aspect` | F64 | aspect ratio of the current frame |
| `render.interp_fps` | U32 | frame rate shown: 30, the interpolation rate, or 60 in true 60 mode |
| `render.true60` | BOOL | 0 or 1 |

```c
static char layout[16];
if (wwhd_setting_get("input.face_layout", WWHD_SETTING_STRING, layout, sizeof layout)) {
    /* layout holds the current preset name */
}
```

## Drawing on screen (HUD API v2)

Register one draw callback with `wwhd_hud_register(draw, screen)` from a game hook (a null callback
unregisters it). The port calls `draw(list)` after each logic step; record elements with
`wwhd_hud_emit(list, &element)`. The list is shown when the callback returns and stays on screen
until the next step. Only read game state in the callback.

```c
static wwhd_hud_element box;
static void draw(u32 list) {
    box = (wwhd_hud_element){
        .kind = WWHD_HUD_RECT, .anchor = WWHD_HUD_TOP_LEFT,
        .x = 20, .y = 96, .w = 160, .h = 40,
        .thickness = 1, .u1 = 1, .v1 = 1, .rgba = 0x204060C0
    };
    wwhd_hud_emit(list, &box);
}
WWHD_HOOK(WWHD_ADDR_daPy_Execute, register_box, (void* link)) {
    (void)link;
    wwhd_hud_register(draw, WWHD_HUD_BOTH);
}
```

- **Elements** (`wwhd_hud_element`): filled or outlined rectangles and circles, lines, text and
  images. Set `thickness`, `u1` and `v1` to 1 unless you need other values. Circle `size` is the
  radius, text `size` the height. Images support UV subrects, tint, rotation and alpha or additive
  blending. Keep elements and text in static storage or your heap.
- **Coordinates:** TV 1280 × 720, GamePad 854 × 480 (`WWHD_HUD_BOTH` uses TV coordinates, scaled).
  Anchors keep elements at the screen edges on wide screens.
- **Images:** `wwhd_hud_texture(WWHD_HUD_PACKAGE, "assets/x.png")` loads your package's own art;
  `WWHD_HUD_DATA` loads from your mod's data folder (for images your setup tool prepared). Keep the
  handle and free it with `wwhd_hud_release`. After a state load the handles are gone: compare
  `wwhd_hud_epoch()` in each callback and reload when it changes.
- **Clipping:** `wwhd_hud_clip(list, &element)` records `WWHD_HUD_CLIP_PUSH` (a screen-aligned
  rectangle from `x/y/w/h` and `anchor`, intersected with the parent clip) and `WWHD_HUD_CLIP_POP`.
  Up to 16 nested clips; an unbalanced callback drops the list.
- **Limits:** per list 1024 elements and 64 KiB of text; per mod 32 images and 16 MiB of decoded
  pixels. An invalid element (bad UTF-8, bad handle, non-finite numbers, over the limits) drops the
  whole list, with a message in the Mods tab.

It draws on Metal and Vulkan, on the TV picture and the GamePad screen; the settings overlay stays
on top. Positions are not interpolated between logic steps.

## PCM audio streams

Mods may play original synthesized audio: `wwhd_audio_open(48000, channels)` (one or two channels),
`wwhd_audio_submit(handle, samples, frames, channels)`, `wwhd_audio_available(handle)` and
`wwhd_audio_close(handle)`. Samples are signed 16-bit, interleaved and big-endian (the game CPU's
order); the host copies them before returning. Only 48000 Hz is supported.

- Each stream holds 8192 frames; each submission is at most 2048 frames; four streams per mod.
- Submit returns the accepted frames (zero when full). Errors: `WWHD_AUDIO_INVALID` (-1),
  `WWHD_AUDIO_BUSY` (-2: retry later without advancing your synthesis), `WWHD_AUDIO_QUOTA` (-3).
- Compare `wwhd_audio_epoch()` before using handles: loading a full save state or an output device
  change clears all streams. Recreate them and resume from your own musical state.

## Trust, save states and limits

- **Trust.** A code mod becomes native code inside the game process. Its memory accesses stay inside
  the game's memory and it reaches the system only through the services above, but that is not a
  sandbox: it can crash the game or damage saves. Enabling a code mod asks once; a changed package
  asks again.
- **Save states.** Full save states include the mods' memory and heaps. Loading a state made with
  different mods shows a warning and continues. Files a mod writes itself are not part of save
  states.
- **Limits.** Changing enabled mods or their options needs a restart. Mods live in a 16 MiB region of
  game memory; each mod gets its own part. No events or exports between mods yet. Instructions the
  translator doesn't support are reported when the mod is built.

Next: [Story mods](story.md).
