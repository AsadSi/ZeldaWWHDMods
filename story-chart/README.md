# Old map

A treasure chart on its own: no quest and no notebook entry, and no code needed. Everything is
data in `manifest.json`: the chart (`charts`, with its spot and the item in its chest) and the
sailor's letter that brings it (`letters`). Needs port 0.3.0 and the Story Notebook package (the
port's story system).

What the player gets:

1. **The letter:** once Link has the Delivery Bag, a sailor's letter waits at any Rito postbox.
2. **The MAP list:** after reading it, Link owns the Old map. The MAP screen's chart list shows it
   after the game's charts.
3. **Open the chart once:** it shows the X at its spot in the open sea south-west of Windfall
   (square D2). Like a game treasure chart, an owned chart puts nothing on the sea chart.
4. **The light ring:** once the chart has been opened, the game's ring of light marks the spot
   while Link sails there.
5. **The crane:** in the ring, Link uses the grappling hook from the boat, as for the game's
   treasures, and brings up the chest.
6. **The chest:** it holds a purple rupee (the chart's `"item": "game:purple_rupee"`).
7. **The red circle:** the chart is finished, and the game's red hand-drawn circle marks the spot on
   the sea chart and in the chart view.

The save file remembers the letter, the chart and whether it is finished. The code only logs the
chart's state (`wwhd_chart_state`) while testing.

See [Charts and markers](../docs/sdk/story.md#charts-and-markers),
[Letters](../docs/sdk/story.md#letters) and [the worked example](../docs/sdk/examples.md#the-old-map).
The texts are original.
