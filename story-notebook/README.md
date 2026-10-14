# The Story Notebook

The story system's required package. The port installs and enables it together with its story system;
every quest mod depends on it.

When it is on, a storyteller's letter waits in the Rito postbox (once Link has the Delivery Bag). The
letter brings the story notebook, the journal of all story mods, and a sealed letter for the
storyteller's old friend who keeps Windfall's light: the first small quest, "The storyteller's
lantern", described as data in `manifest.json` (`quests`).

- Open the notebook from the Delivery Bag; choose the quest and "Show on chart" to see where to go.
- Talk to the man up on the windmill by the light (`windfall-um1` in the port's NPC catalogue; he is
  there at any time): he reads the letter and thanks Link with an orange rupee (100), and the quest
  moves to the notebook's archive. He says the quest's lines while Link carries the letter; otherwise
  he talks as in the game.
- While the quest is shown on the chart, the King of Red Lions reminds Link once where the letter goes
  (`sail_line`).

The manifest's `notebook` block gives the notebook's look:

- `icon` and `icon_open`: the notebook in the Delivery Bag, closed and open (PNG);
- `model`: its 3D model (binary glTF); the port builds the game model from it on the player's machine;
- `layout`: the notebook page (`assets/notebook.json`, a layout description naming the page's pictures
  next to it); the port builds the page from it on the player's machine.

All pictures and the model are original artwork, listed with their author and licence in
[assets/ASSETS.md](assets/ASSETS.md). The page's texts use the game's own fonts and its mini chart the game's
Link icon; the layout names them and the port takes them from the player's game, so no game file is
part of this package.

The mod's code only logs the quest's state: the port does the rest from the manifest (see
[Quests in the manifest](../docs/sdk/story.md#quests-in-the-manifest)). The texts are original.

Needs a port with the story system (see `port_versions` in the catalogue).
