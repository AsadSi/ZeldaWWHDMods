# Story mods

A story mod adds a small adventure in the game's own way: a letter in the Rito postbox, a treasure
chart, a quest in the story notebook, a resident who speaks your lines. Most of it is data in your
manifest; your code only reports what the port can't see. Story mods need port 0.3.0 or later.

- [The story system and the Story Notebook package](#the-story-system-and-the-story-notebook-package)
- [Letters](#letters)
- [Charts and markers](#charts-and-markers)
- [Items](#items)
- [Quests](#quests)
- [Quests in the manifest](#quests-in-the-manifest)
- [Talking to residents](#talking-to-residents)
- [The King of Red Lions](#the-king-of-red-lions)
- [The NPC catalogue](#the-npc-catalogue)
- [Where story progress is saved](#where-story-progress-is-saved)

## The story system and the Story Notebook package

Letters, quests, the notebook and mod charts are the port's story system (the "Story system"
built-in, on by default). It comes with one package from this repository,
[`story-notebook`](../../story-notebook/): its letter brings the story notebook, its manifest's
`notebook` block gives the notebook's look, and it holds a first small quest. The port downloads it
from the catalogue by itself and keeps it enabled while the story system is on.

Every story mod depends on it: a manifest with `quests` or `charts` gets the dependency even if it
doesn't declare it, but declare it anyway so the catalogue lists it:

```json
"dependencies": [{"id": "story-notebook", "minimum_version": "0.6.0"}]
```

While the package isn't installed (offline, download failed), the story system is inactive: no
letters, chart marks or notebook; story mods wait with "Needs the Story Notebook quest", the game is
untouched and no save is changed.

## Letters

A letter is handed out by any Rito postbox like the game's own mail, and the game shows it in its
letter window. Delivery starts once Link has the **Delivery Bag**; until then a letter waits in the
save file. A mod's letter needs no postage. Several waiting letters are handed out one after another.

A quest's own letter is declared with the quest (see [Quests in the manifest](#quests-in-the-manifest)).
A letter with no quest is declared in the manifest's `letters` block, or sent from code when it
depends on your own conditions.

### Letters in the manifest

A letter needs no code: declare it in `manifest.json` and the port sends it while your mod is on.
Once Link has read it, he has what it encloses: your charts (`"chart:<id>"`) and items
(`"item:<id>"`), each given once. Such a letter makes no entry in the story notebook.

```json
"letters": [
  {"id": "map-letter", "sender": "A sailor",
   "text": "Dear Link,\nI found this old map in a bottle.\nMaybe you can find what it marks.",
   "encloses": ["chart:old-map"]}
]
```

Up to 8 letters per mod, with the [rules for letters](#rules-for-letters) below. Every chart and item
a letter encloses must be declared in the manifest, and a letter's ID must not be a quest letter's.

### Letters from code

```c
s32 state = wwhd_mail_send("hello", "A wandering storyteller",
                           "Dear Link,\na new tale has reached the Great Sea.\nFair winds!");
```

`wwhd_mail_send(letter_id, sender, text)` puts the letter in the postbox of the loaded save file,
unless that file already got it. Call it from a gameplay hook; calling it every logic step never
sends it twice. Use it for a letter that depends on your own conditions (Call of the Sky sends its
letter once Link has helped the dragon). `wwhd_mail_state(letter_id)` returns the state without
sending.

| Result | Meaning |
| --- | --- |
| `WWHD_MAIL_NONE` (0) | never sent to this save file (only from `wwhd_mail_state`) |
| `WWHD_MAIL_WAITING` (1) | waiting in the postbox |
| `WWHD_MAIL_READ` (2) | handed out and read |
| `WWHD_MAIL_INVALID` (-1) | invalid ID, sender or text |
| `WWHD_MAIL_NO_ROOM` (-2) | no save file loaded, or its story record is full |

A letter from code encloses a green rupee.

### Rules for letters

- **ID:** 1 to 32 characters `a`-`z`, `0`-`9`, `-`, `_`, `.`; unique within your mod.
- **Sender:** one line, up to 48 bytes, shown below the text as the signature.
- **Text:** plain UTF-8, up to 1200 bytes; `\n` breaks a line. Other control characters are refused.
  Keep lines short (about 40 characters) and the letter to about ten lines. Characters the game's
  font lacks are not shown.
- The text is not stored in the save; turning your mod off leaves its letters waiting.

## Charts and markers

A mod's chart works like the game's own treasure charts. While Link only owns it, the sea chart
shows nothing for it. Once he has opened it, the game's ring of light marks its spot while he sails
there; the grappling hook brings up a chest with the chart's item, and the chart is finished: the
game's red hand-drawn circle (the mark of a salvaged treasure) marks the spot.

### Charts in the manifest

Declared charts are charts like the game's: while Link owns one, the MAP screen's chart list shows
it after the game's charts, with its name, and opening it shows the game's chart view with the X at
its spot.

```json
"charts": [
  {"id": "old-map", "name": "Old map",
   "description": "An old map. An X marks a spot in the open sea south-west of Windfall.",
   "place": {"x": -3500, "z": -194000}, "item": "game:purple_rupee"}
]
```

- **Declaring:** `id`, `name` (up to 40 bytes, the list's caption), `description` (up to 200 bytes),
  `place` (the X and the treasure's spot) and optionally `item`. Up to 8 per mod.
- **The chest's item:** `item` is one of your [items](#items) (`"item:<id>"`) or a game item:
  `"game:purple_rupee"`, `"game:orange_rupee"`, `"game:silver_rupee"`, `"game:heart_piece"` or
  `"game:heart_container"`. Without it, the chest holds an orange rupee.
- **Getting it:** a letter that encloses `"chart:<id>"` (a [letter in the manifest](#letters-in-the-manifest)
  or a quest's letter); a resident's talk with `"then": {"give": "chart:<id>"}`; a quest's reward
  (`"reward": {"chart": "chart:<id>"}`); or your code (`wwhd_chart_give`).
- **The treasure:** once Link has opened the chart in the chart list, the light ring shows at its
  spot while he sails there. In the ring, the grappling hook from the boat works as the game's crane
  and brings up the chest. Opening the chest gives its item and finishes the chart.
- **Finishing it otherwise:** a quest step with `"find": "chart:<id>"` finishes the chart once the
  quest is past that step; or your code calls `wwhd_chart_finish`.
- **Where to put the spot:** in open water, away from islands and reefs, so the boat can reach the
  ring. The chart view shows the part of the sea square around the spot, with the X on it, on the TV
  and the GamePad.
- **A treasure chart on its own** needs no quest and no notebook entry: declare the chart and a
  letter that encloses it, as the [Old map](examples.md#the-old-map) does. A quest whose only step is
  to find a chart gets a warning in the log: make it a chart with a letter instead. A chart to find as
  one step of a longer quest is fine.

### Charts from code

| Service | What it does |
| --- | --- |
| `wwhd_chart_give(chart)` | Link gets the chart in the loaded save file (once) |
| `wwhd_chart_finish(chart)` | the chart is done: the red circle marks its markers' spots |
| `wwhd_chart_state(chart)` | the chart's state, without changing it |
| `wwhd_chart_mark(chart, marker, x, z, icon, label)` | adds the marker, or moves it if it exists |
| `wwhd_chart_unmark(chart, marker)` | removes the marker; `NULL` removes all markers of the chart |

They return `WWHD_CHART_NONE` (0), `WWHD_CHART_OWNED` (1), `WWHD_CHART_DONE` (2),
`WWHD_CHART_INVALID` (-1) or `WWHD_CHART_NO_ROOM` (-2).

- **Place:** `x` and `z` in the Great Sea's world coordinates, the ones Link's position uses. The sea
  is 7 × 7 squares of 100000 units; the center of square D4 is (0, 0), `x` grows to the east and `z`
  to the south. `WWHD_CHART_COLUMN_X('A'..'G')` and `WWHD_CHART_ROW_Z(1..7)` give the center of a
  square: Windfall Island (D2) is `WWHD_CHART_COLUMN_X('D'), WWHD_CHART_ROW_Z(2)`.
- **Icon:** `WWHD_CHART_ICON_STORY` is the only one for now.
- **Label:** one line of UTF-8, up to 64 bytes, or `NULL`; it names the marker for other screens.
- **Markers** are placed every session: call `wwhd_chart_mark` from a gameplay hook. Up to 8 markers
  per mod, 32 in all.
- IDs follow the letter rules; marker IDs are unique within their chart.

## Items

A story mod can give Link its own items. They are kept in the Delivery Bag like the game's delivery
items, but they never take one of the bag's eight places unless the player puts one there: the bag
gets extra pages for them.

```json
"items": [
  {"id": "old-key", "name": "Old key", "description": "A rusty key. It smells of the sea.",
   "icon": "assets/old_key.png"},
  {"id": "lamp", "name": "Storm lamp", "description": "Shows the way in the dark.",
   "icon": "assets/lamp.png", "stack": 3, "use": {"action": "show"}}
]
```

| Field | Meaning |
| --- | --- |
| `id` | 1 to 32 characters `a`-`z`, `0`-`9`, `-`, `_`, `.`; unique within your mod. Up to 32 items per mod |
| `name`, `description` | what the item's Info box shows: the name up to 40 bytes, the description up to 200 bytes (line breaks allowed) |
| `icon` | a PNG in your package, best 128 × 128 with transparency (see [Art](art.md)). The port adds it to the game's item pictures on the player's machine, so it shows in the bag and on the buttons like a game item |
| `stack` | how many Link can carry, 1 to 254 (default 1); a give beyond it keeps the maximum |
| `use` | optional: what using the item does. Only an item with a use can go on X, Y or R |

The use actions:

- `{"action": "show"}`: shows its Info box;
- `{"action": "open_notebook"}`: opens the story notebook;
- `{"action": "quest_step", "quest": "<quest id>", "step": 2}`: that quest of your manifest reaches
  the step;
- `{"action": "event"}`: your code is told (`wwhd_item_used`).

**Giving items from the manifest.** A letter can enclose an item (`"encloses": ["item:old-key"]`), a
resident's talk can give one (`"then": {"give": "item:old-key"}`), a quest's reward can be one
(`"reward": {"item": "item:old-key"}`) and a chart's chest can hold one (`"item": "item:old-key"`).
Every item the manifest names must be declared in `items`.

**From code:**

| Service | What it does |
| --- | --- |
| `wwhd_item_give(item, count)` | Link gets `count` of the item (up to its `stack`) |
| `wwhd_item_take(item, count)` | takes `count`; `WWHD_ITEM_NOT_ENOUGH` (-3) if Link has fewer |
| `wwhd_item_count(item)` | how many Link has |
| `wwhd_item_used(item)` | for an item with the `event` use: how often the player used it since the last call |

They return the count now (0 to 254) or an error: `WWHD_ITEM_INVALID` (-1: not an item of your
manifest, or a count of 0 or over 254) or `WWHD_ITEM_NO_ROOM` (-2: no save file loaded, or its story
record is full).

```c
s32 n = wwhd_item_give("lamp", 1);
if (wwhd_item_count("old-key") > 0) wwhd_item_take("old-key", 1);
if (wwhd_item_used("token") > 0) { /* the player used it since the last call */ }
```

**What the player sees.** When Link gets an item, its name and description show with the game's
small get jingle. In the Delivery Bag window, R (or a tap on the page dots above the window) turns to
the extra pages and L back; B returns to the game's eight places. A on an item offers Info, Use and a
move into an empty place of the game's eight. Like the game's items, an item with a use can be put
on X, Y or R: in the Delivery Bag, X or Y puts it on that button (R there turns the pages while Link
has mod items). When the bag's pages are full, a new item is kept and gets a place
as soon as one is free; the caption says so.

**Saving.** How many of each item Link has, where it sits in the bag and which button it is on are
kept in the save's story block, never in the game's own item data. A save without your mod is a
normal save: the bag shows the game's items only, and the buttons keep the game's items. Turning your
mod on again brings its items back.

## Quests

The story notebook is the port's journal for story mods. It comes with the Story Notebook package's
letter and is kept in the Delivery Bag. Opening it shows one row per quest: the title and your mod's
name, the next step, where, and the status (**New**, **In progress**, **Not yet available**, or
**Done** in the Archive tab). Choosing a quest shows what is known so far and the reward, and
**Show on chart** puts the game's story ring on the next step's place on the sea chart (one quest at
a time). A quest that hasn't started yet can be shown too: its page says "Not started yet" with step
1's place, and the ring goes there.

The preferred way to describe a quest is data in the manifest (next section). From code:

```c
WWHD_HOOK(WWHD_ADDR_daPy_Execute, quest_step, (void* link)) {
    (void)link;
    u16 max_life = dComIfGs_player_status_a.max_life;
    if (max_life < 12 || max_life > 80) return;  /* no save file loaded */
    s32 letter = wwhd_mail_send("hello", "A wandering storyteller", "Dear Link, ...");
    s32 quest = wwhd_quest_define("tale", "The storyteller's tale", "A piece of heart", 0);
    if (letter == WWHD_MAIL_READ && quest == WWHD_QUEST_NEW) quest = wwhd_quest_step("tale", 1, 0, 0);
    if (quest == 1) {
        wwhd_quest_step("tale", 1, "Ask the storyteller about his lantern.", "Windfall Island");
        wwhd_quest_place("tale", WWHD_CHART_COLUMN_X('D'), WWHD_CHART_ROW_Z(2));
    }
}
```

| Service | What it does |
| --- | --- |
| `wwhd_quest_define(quest, title, reward, condition)` | registers a code-defined quest for this session (call it first, in every session) |
| `wwhd_quest_step(quest, step, next, where)` | step N reached (1..254), with what to do next and where; step 0 sets the texts only |
| `wwhd_quest_place(quest, x, z)` | the next step's place on the sea chart (chart coordinates) |
| `wwhd_quest_no_place(quest)` | the next step has no place on the chart |
| `wwhd_quest_known(quest, text)` | what the player knows so far (several lines) |
| `wwhd_quest_portrait(quest, png)` | the quest's portrait, a PNG of your package (`assets/`, up to 256 × 256) |
| `wwhd_quest_done(quest)` | the quest is done |
| `wwhd_quest_state(quest)` | the quest's state, without changing it |
| `wwhd_quest_set_flag(quest, n, on)` / `wwhd_quest_flag(quest, n)` | your own progress flags 1..8 inside a step |
| `wwhd_quest_carry(quest, carried)` | gives (1) or takes (0) the quest's sealed letter |
| `wwhd_quest_talk(quest, step, resident, lines, reward)` | a resident's talk for that step, see [Talking to residents](#talking-to-residents) |

They return `WWHD_QUEST_NEW` (0), the step reached (1 to 254), `WWHD_QUEST_DONE` (255),
`WWHD_QUEST_INVALID` (-1) or `WWHD_QUEST_NO_ROOM` (-2).

- **IDs** follow the letter rules. Up to 8 quests per mod.
- **Texts** are plain UTF-8, one line each, `NULL` for none: title (1 to 48 bytes), reward (96),
  condition (96), next step (120: one sentence), where (48: the island or place). What is known may
  have line breaks (600 bytes). Texts are registered every session, not saved.
- **Steps** only go forward: a lower step than the saved one changes nothing, and a done quest stays
  done. Calling the services every logic step is fine.
- **Condition:** a quest that can't be started yet names its condition; the notebook shows
  "Not yet available: ..." until your mod reaches step 1.

## Quests in the manifest

The port reads the quest when the player enables your mod, without running your code, so the quest
is in the notebook at once. Your code then only reports progress the port can't see
(`wwhd_quest_step`, `wwhd_quest_done`, flags). The Story Notebook's own quest needs no code at all:

```json
"npc_catalogue": 3,
"quests_format": 1,
"quests": [{
  "id": "lantern",
  "title": "The storyteller's lantern",
  "journal": "on_enable",
  "start": {"text": "Check a Rito postbox for a letter."},
  "letter": {"id": "lantern", "sender": "The storyteller", "text": "Dear Link, ...",
             "encloses": ["notebook", "letter:friend"]},
  "reward": {"text": "100 rupees", "item": "rupee_orange"},
  "steps": [{
    "n": 1,
    "text": "Bring the storyteller's letter to his old friend who keeps Windfall's light.",
    "npc": {"who": "windfall-um1",
            "when": {"carries": "letter:friend"},
            "lines": ["A letter from my old friend? Thank you!", "...", "..."],
            "then": {"give": "rupee_orange", "take": "letter:friend", "complete": true}}
  }]
}]
```

| Field | Meaning |
| --- | --- |
| `id`, `title` | the quest's ID and title (rules above) |
| `journal` | `on_enable` (in the notebook as soon as the mod is on, the default), `on_trigger`, or `hidden` (not in the notebook) |
| `start.text` | what the notebook shows before step 1 |
| `letter` | the quest's letter: the port sends it; when it is read the quest reaches step 1. `encloses` may hold `"letter:<id>"` (a sealed letter Link carries for the quest, shown in the Delivery Bag), `"chart:<id>"` and `"item:<id>"`. Only the Story Notebook package may enclose `"notebook"` |
| `reward` | `text` for the notebook; `item` given when the quest is done: a rupee (`rupee_green` ... `rupee_orange`) or one of your items (`"item:<id>"`); or `chart` (`"chart:<id>"`) |
| `steps` | the steps in order, each with `n` (1, 2, ...) and `text`, plus the optional fields below |

A sealed letter enclosure may be written as an object to describe it in your own words:
`{"id": "letter:friend", "name": "Sealed Letter", "description": "A sealed letter from the storyteller for his old friend."}`
(`name` up to 40 bytes, `description` up to 200 bytes and three lines).

Step fields:

| Field | Meaning |
| --- | --- |
| `where` | the place in words (48 bytes) |
| `place` | `{"x", "z"}` in chart coordinates: the tracked quest's ring goes there |
| `find` | `"chart:<id>"`: the step is to find that chart's treasure; the chart is finished once the quest is past this step |
| `available` | prerequisites, checked read-only: `"event"` / `"not_event"` (the game's event flag numbers, `"0x2D01"` or a list) that must be set / not set; `"time"`: `"day"` (6:00–18:00), `"night"` or `"any"`; `"text"` for the player. While they don't hold, the notebook says "Not yet possible: <text>" |
| `npc` | a resident speaks your lines, below. The step takes `where` and `place` from the NPC catalogue unless it gives its own; `"follow": false` keeps the marks at the step's place |
| `sail_line` | what the King of Red Lions says once about this step, below |

A declaration the port doesn't accept (an invalid field, an NPC its catalogue doesn't know, or an
`npc_catalogue` version newer than the port's) keeps the mod from being enabled; the Mods tab shows
the reason ("Story quests: ...").

## Talking to residents

A step can have a game resident speak your lines instead of their own, once: the friend reads the
letter, thanks Link, gives the reward through the game's own get-item scene, and the quest is done.
The port arbitrates, not your mod:

- the talk takes over while your quest is at that step and its conditions hold: Link carries its
  letter (`when.carries`) and the step's `available` holds. Otherwise the resident talks exactly as
  in the game;
- several quests may want the same resident at once: the talk then starts with the game's own
  choice box (two quests and "Never mind"). One quest per talk;
- a step that takes over a resident with no condition at all speaks for your quest whenever the step
  is current; the log warns about it, so give such steps a condition;
- the port never sets or clears a game flag;
- the lines are one message box: up to three lines, 240 bytes. `then.give` is a rupee item, one of
  your charts (`"chart:<id>"`) or one of your items (`"item:<id>"`);
  `then.take` takes the sealed letter; `then.complete` finishes the quest;
- `"who"` names a resident by its neutral id from the [NPC catalogue](#the-npc-catalogue).

From code, the same is `wwhd_quest_talk(id, step, "windfall-um1", lines, reward_item)`, and
`wwhd_quest_carry(id, 1)` gives the sealed letter.

## The King of Red Lions

A step's `sail_line` is said once by the boat, the way he gives his own news: the next time Link
boards him or walks up to him, he says it in one message box (a string, or a list of up to three
lines, 240 bytes).

```json
{"n": 1, "text": "Bring the storyteller's letter to his old friend who keeps Windfall's light.",
 "sail_line": ["That sealed letter you carry is for the man",
               "who keeps the light up on Windfall's windmill.",
               "Let us not keep an old friend waiting, Link!"]}
```

- Only for the quest shown on the chart, at its current step.
- The game comes first: his own lines, events and cutscenes always go before the quest's line, and
  talking to him yourself always gives his own hint.
- Said once per step; the save's story record remembers it, never a game flag.

## The NPC catalogue

Quests name the game's characters by a **neutral id**, `<place>-<code>` (`windfall-um1`), never by
their names. The port ships a recipe per place with our own description of each character; the
character's name and where it stands come from the player's own game files.

**Which id is which character.** The port includes a lookup tool. Run it from a checkout of the
port repository against your own game:

```sh
python3 tools/story_mods/npc_catalogue.py --game <your game folder> --list
```

It prints each id with the name your game gives the character, where it is and its spot. That list
exists only on your machine: keep names out of your manifest, README and commits, and use the ids.

A mod that names a character added in a later catalogue version declares the version it needs,
`"npc_catalogue": 3`, so an older port refuses it with a clear message.

Catalogue version 3, Windfall Island. Only the townspeople can speak a quest's lines so far.

| Id | Who (our words) | Where | Out | Quest lines |
| --- | --- | --- | --- | --- |
| `windfall-uo1` | an old man resting on the sea-view bench at the cliff | on the sea-view bench at the cliff | day | yes |
| `windfall-uo2` | a man who sits at the bar all day | in the bar | day | yes |
| `windfall-uo3` | a well-dressed man walking through town | walking through town | day | yes |
| `windfall-ub1` | one of the two women chatting up the steps by the brick house (the first) | up the steps by the brick house | day | yes |
| `windfall-ub2` | one of the two women chatting up the steps by the brick house (the second) | up the steps by the brick house | day | yes |
| `windfall-ub3` | a woman on the harbour pier | on the harbour pier | day | yes |
| `windfall-ub4` | a woman by the town gate | by the town gate | day and night | yes |
| `windfall-uw1` | the young woman working in the bar | in the bar | day and night | yes |
| `windfall-uw2` | a young woman in the town square or in the bar | the town square or the bar | day | yes |
| `windfall-um1` | the man up on the windmill by the light | up on the windmill by the light | day and night | yes |
| `windfall-um2` | a young man in the town square or in the bar | the town square or the bar | day | yes |
| `windfall-um3` | a young man on the steps by the red door (up high at night) | on the steps by the red door | day and night | yes |
| `windfall-sa1` | a sailor who drinks in the bar at night | in the bar at night | night | yes |
| `windfall-sa2` | a sailor on the high lookout | on the high lookout | day | yes |
| `windfall-sa3` | a sailor on the stone dock | on the stone dock | day and night | yes |
| `windfall-sa4` | a sailor at the end of the wooden pier | at the end of the wooden pier | day and night | yes |
| `windfall-sa5` | a sailor on the grass by the stone wall | on the grass by the stone wall | day and night | yes |
| `windfall-ug1` | one of the two by the flowers at the cliff (the first) | by the flowers at the cliff | day and night | yes |
| `windfall-ug2` | one of the two by the flowers at the cliff (the second) | by the flowers at the cliff | day and night | yes |
| `windfall-rsh1` | the travelling merchant at his stall | the travelling merchant's stall | day | not yet |
| `windfall-tt` | the dancer on the hill | dancing on the hill | day and night | not yet |
| `windfall-kk1` | the rich man's daughter | in town | day and night | not yet |
| `windfall-gk1` | the rich man, the daughter's father | in town | day and night | not yet |
| `windfall-kp1` | the poor family's daughter | the auction house | day and night | not yet |
| `windfall-pf1` | the poor family's father | in town | day and night | not yet |
| `windfall-bms1` | the bomb shop keeper | the bomb shop | day and night | not yet |
| `windfall-ds1` | the potion maker in his shop | the potion shop | day and night | not yet |
| `windfall-po` | the photographer in his studio | the photographer's studio | day and night | not yet |
| `windfall-ho` | the teacher of the school house | the school house | day and night | not yet |
| `windfall-kg1` | the host of the battleship game | the battleship game | day and night | not yet |
| `windfall-npcso` | the talking fish in the sea by the island | the sea by the island | day and night | not yet |
| `windfall-tc` | the man locked in the jail | the jail | day and night | not yet |
| `windfall-bm4` | a man in the bar who also comes to the auctions | the bar and the auction house | day and night | not yet |
| `windfall-zl1` | the pirate captain in the bomb shop | the bomb shop | day and night | not yet |
| `windfall-p1a` | a pirate in the bomb shop (the first of three) | the bomb shop | day and night | not yet |
| `windfall-p1b` | a pirate in the bomb shop (the second of three) | the bomb shop | day and night | not yet |
| `windfall-p1c` | a pirate in the bomb shop (the third of three) | the bomb shop | day and night | not yet |
| `windfall-mk` | one of the four children around town (the leader) | around town | day | not yet |
| `windfall-ukb` | one of the four children around town (the second) | around town | day | not yet |
| `windfall-ukc` | one of the four children around town (the third) | around town | day | not yet |
| `windfall-ukd` | one of the four children around town (the fourth) | around town | day | not yet |

"By day" is 6:00–18:00; during the Endless Night it is always night. A step whose `available` time
never meets its character's is accepted with a warning in the log.

## Where story progress is saved

Everything the port keeps for story mods (letters, charts, the notebook, quests, their flags, the
tracked quest, carried letters) is stored per Quest Log in a story block appended to the game's save
file, after the game's own bytes. The game never sees it, and it is written with every game save, so
story progress always matches the game save: progress not saved is undone by loading, like the
game's own. Full save states carry it. Without any story progress nothing is appended, and with all
mods off the save is a plain game save. The block holds up to 255 records per Quest Log.

Turning your mod off leaves its records in the save; the notebook and the sea chart simply don't
show them.

Next: [Art, layouts and licences](art.md).
