/* Example guest mod: the Old map, a treasure chart on its own (no quest, no notebook entry).
 *
 * Everything is data in manifest.json: the chart ("charts", with its spot and the item in its chest)
 * and the sailor's letter that brings it ("letters"). The port sends the letter, gives Link the chart
 * once he has read it, puts the light ring at the spot once he has opened the chart, and finishes the
 * chart when he opens the chest the crane brings up. The code only logs the chart's state as it
 * changes, which helps while testing. All texts are original to this example. */
#include "wwhd_guest.h"
#include "wwhd/functions.h"
#include "wwhd/save.h"

static u32 steps;
static s32 last = -100;

WWHD_HOOK(WWHD_ADDR_daPy_Execute, story_chart_step, (void* link)) {
    (void)link;
    if (++steps % 30) return; /* twice a second is plenty */
    u16 max_life = dComIfGs_player_status_a.max_life;
    if (max_life < 12 || max_life > 80) return; /* no save file loaded */
    s32 state = wwhd_chart_state("old-map");
    if (state != last) {
        last = state;
        wwhd_log_int("story-chart: Old map state (0 not yet, 1 owned, 2 finished)", state);
    }
}
