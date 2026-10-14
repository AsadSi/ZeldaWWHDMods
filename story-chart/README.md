# Old map

A treasure chart on its own: no quest and no notebook entry. The manifest declares the chart
(`charts`); the code sends the letter that brings it.

- When the mod is on, a sailor's letter waits in the Rito postbox (once Link has the Delivery Bag).
- After reading it, Link owns the Old map (`wwhd_mail_send`, then `wwhd_chart_give` once
  `wwhd_mail_send` reports the letter read). The MAP screen's chart list shows it after the game's
  charts, and opening it shows the X at the top of Windfall's lighthouse.
- While Link only owns the chart, the sea chart shows nothing for it, as for an owned treasure chart.
- Climb to the top of the lighthouse: the mod finishes the chart (`wwhd_chart_finish`), and the
  game's red hand-drawn circle marks the spot on the sea chart and in the zoomed square.

See [Charts and markers](../docs/sdk/story.md#charts-and-markers) and
[Letters](../docs/sdk/story.md#letters). The texts are original. Needs the Story Notebook package
(the port's story system).
