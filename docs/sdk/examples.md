# Worked examples

Three story packages of this repository, from the smallest to the largest. Each folder is a complete
mod: read its `manifest.json`, `src/mod.c` and README side by side with this page.

## The Old map

[`story-chart/`](../../story-chart/): a treasure chart on its own, no quest and no notebook entry.

**The manifest** declares the chart and the letter that brings it:

```json
"dependencies": [{"id": "story-notebook", "minimum_version": "0.6.0"}],
"charts": [
  {"id": "old-map", "name": "Old map", "description": "An old map. An X marks the top of the lighthouse.",
   "place": {"x": 1759, "z": -202572}}
],
"letters": [
  {"id": "map-letter", "sender": "A sailor",
   "text": "Dear Link,\nI found this old map in a bottle.\nMaybe you can find what it marks.",
   "encloses": ["chart:old-map"]}
]
```

That is all the player needs to receive it: once Link has the Delivery Bag, the sailor's letter
waits at any Rito postbox; after reading it, Link owns the Old map, and the MAP screen's chart list
shows it with its X at the top of Windfall's lighthouse. The sea chart shows nothing for it yet, as
for an owned treasure chart.

**The code** finishes the chart when Link stands on its spot:

```c
s32 state = wwhd_chart_state("old-map");
const fopAc_ac_c* actor = (const fopAc_ac_c*)link;
f32 dx = actor->current.pos.x - SPOT_X, dz = actor->current.pos.z - SPOT_Z;
if (state == WWHD_CHART_OWNED && dx * dx + dz * dz < 600.0f * 600.0f) state = wwhd_chart_finish("old-map");
```

From then on, the game's red hand-drawn circle marks the spot on the sea chart. The save file
remembers the chart and whether it is finished.

What to take from it: a letter and a chart need no code; `charts[].place` and the sea's world
coordinates are the same as Link's position; finishing is up to your mod.

## The Story Notebook

[`story-notebook/`](../../story-notebook/): the story system's required package and a quest with no
code at all.

**The quest** "The storyteller's lantern" is data in the manifest:

- its `letter` encloses `"notebook"` (only this package may) and a sealed letter
  (`"letter:friend"`, described in its own words). Reading it brings the notebook and starts the
  quest at step 1;
- its one step names a resident by neutral id, `windfall-um1` (the man up on the windmill by the
  light; see [the NPC catalogue](story.md#the-npc-catalogue)). While Link carries the sealed letter,
  that resident says the quest's three lines, takes the letter, gives an orange rupee and completes
  the quest (`then`);
- the step's `sail_line` has the King of Red Lions remind Link once where the letter goes, while the
  quest is shown on the chart;
- `"npc_catalogue": 3` says which catalogue version the quest needs.

**The look** is the `notebook` block: the Delivery Bag icons (closed and open), the 3D model and the
page layout with its pictures, all in `assets/` and listed in `assets/ASSETS.md`. See
[Art, layouts and licences](art.md#the-story-notebooks-look).

**The code** only logs the quest's state with `wwhd_quest_state("lantern")`, which is a good way to
watch a manifest quest while testing.

## Call of the Sky

[`dragon/`](../../dragon/): a larger mod that mixes a manifest quest with its own gameplay code: an
optional song quest on Dragon Roost Island and a ride across the Great Sea. It is marked
experimental.

**The quest** "Valoo's Gratitude" is declared in the manifest (`quests`): title, start text, reward
and three steps, each with `where` and a `place` on the island, so the notebook shows it and can put
it on the chart.

**The letter comes from code,** because it depends on the game: only once Link has the Deku Leaf and
has helped the dragon does the mod call `wwhd_mail_send("medli", ...)`; when `wwhd_mail_state`
reports it read, the quest reaches step 1.

**Progress is reported from code:** the mod's own state machine (meeting at the high entrance,
striking three wind chimes, conducting the song) maps to the quest's steps with `wwhd_quest_step`,
the three chimes to quest flags 1 to 3 with `wwhd_quest_set_flag`, and the end to `wwhd_quest_done`.
Reading the record back (`wwhd_quest_state`, `wwhd_quest_flag`) after a state load keeps the mod in
step with the save, so progress follows the Quest Log, its copies and save states.

**Structure:** one translation unit `src/mod.c` that includes implementation headers (`*_impl.h`)
for the quest, ride, HUD and persistence; pure logic (state transitions, flight maths, HUD layout)
lives in plain headers that `tests/` compiles for the host and checks with
`python3 -m unittest discover -s dragon/tests -p 'test_*.py'`.

What to take from it: describe the quest as data, keep the code for what only it can know, and make
pure logic testable on the host.
