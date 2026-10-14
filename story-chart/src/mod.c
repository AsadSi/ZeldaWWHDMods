/* Example guest mod: the Old map, a treasure chart on its own. The chart and the letter that brings it
 * are data in manifest.json ("charts", "letters"): the port sends the letter, and once Link has read it
 * he owns the chart. Like an owned treasure chart, it puts nothing on the sea chart. This code only
 * finishes the chart when Link stands on its spot (the top of Windfall's lighthouse, square D2): then
 * the game's red circle (the mark of a salvaged treasure) marks the spot on the sea chart. The save file
 * remembers the chart and whether it is finished. All texts are original to this example. */
#include "wwhd_guest.h"
#include "wwhd/actor.h"
#include "wwhd/functions.h"
#include "wwhd/save.h"

#define SPOT_X 1759.0f
#define SPOT_Z (-202572.0f)

static u32 steps;
static s32 last = -100;

WWHD_HOOK(WWHD_ADDR_daPy_Execute, story_chart_step, (void* link)) {
    if (++steps % 15) return;  /* four times a second is plenty */
    u16 max_life = dComIfGs_player_status_a.max_life;
    if (max_life < 12 || max_life > 80) return;  /* no save file loaded */
    s32 state = wwhd_chart_state("old-map");
    /* Link on the spot (the sea's world coordinates): the chart is done */
    const fopAc_ac_c* actor = (const fopAc_ac_c*)link;
    f32 dx = actor->current.pos.x - SPOT_X, dz = actor->current.pos.z - SPOT_Z;
    if (state == WWHD_CHART_OWNED && dx * dx + dz * dz < 600.0f * 600.0f) state = wwhd_chart_finish("old-map");
    if (state != last) {
        last = state;
        wwhd_log_int("story-chart: Old map state (0 not yet, 1 owned, 2 finished)", state);
    }
}
