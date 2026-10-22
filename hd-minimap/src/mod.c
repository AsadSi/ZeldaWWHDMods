/* HD minimap: a corner map on the TV drawn with the HD game's own GamePad map art and rules.
 * The map art (island maps, dungeon floor plans, treasure charts, markers) is read by the port from the
 * player's installed game at run time (WWHD_HUD_GAME); this mod only names layouts, textures and panes.
 * The frame and the wind arrow are Minimap HD artwork by Chironeyes (MIT, see NOTICE.md).
 * Read-only: no writes to game memory. */
#include "wwhd_guest.h"
#include "wwhd/actor.h"
#include "wwhd/functions.h"
#include "wwhd/data.h"
#include "hd_minimap.h"

/* game functions (USA addresses; the port translates them for EU) */
WWHD_GAME_FUNC(WWHD_ADDR_dScnPly_isPause, s32, game_paused, (void));
WWHD_GAME_FUNC(0x025B8568, s32, sector_discovered, (u32 player_map, s32 cell));        /* dSv_player_map_c::isSaveArriveGrid */
WWHD_GAME_FUNC(WWHD_ADDR_dSv_player_map_c_isGetMap, s32, chart_owned, (u32 player_map, s32 index));
WWHD_GAME_FUNC(WWHD_ADDR_dSv_player_map_c_isOpenMap, s32, chart_open, (u32 player_map, s32 index));
WWHD_GAME_FUNC(WWHD_ADDR_dSv_memBit_c_isDungeonItem, s32, dungeon_item, (u32 membit, s32 item));
WWHD_GAME_FUNC(WWHD_ADDR_dSv_memBit_c_isVisitedRoom, s32, room_visited, (u32 membit, s32 room));
WWHD_GAME_FUNC(WWHD_ADDR_dSv_memBit_c_isTbox, s32, chest_open, (u32 membit, s32 chest));
WWHD_GAME_FUNC(WWHD_ADDR_dComIfGs_isStageTbox, s32, stage_chest_open, (s32 stage, s32 chest));
WWHD_GAME_FUNC(WWHD_ADDR_dComIfGs_isStageMap, s32, stage_map, (s32 stage));
WWHD_GAME_FUNC(WWHD_ADDR_dComIfGs_isStageCompass, s32, stage_compass, (s32 stage));
WWHD_GAME_FUNC(0x0258DC48, u8, stage_floor, (void* stage, f32 y));                     /* dMap_GetFloorNo */
WWHD_GAME_FUNC(0x0258DBF4, u8, room_floor, (s32 room, f32 y));                         /* dMap_GetFloorNo_WithRoom */
WWHD_GAME_FUNC(0x0257DACC, u32, wind_angle, (void));                                   /* dKyw_get_wind_angle */
WWHD_GAME_FUNC(0x025C11DC, u8*, room_data, (u32 control, s32 room));
WWHD_GAME_FUNC(0x025C4734, u8*, stage_treasures, (void));

/* play (dComIfGp) and save offsets used here */
#define PLAY_STAGE 0x5150           /* dStage_stageDt_c: +0x48 stage info, +0x74 DMAP */
#define PLAY_PARTNER 0x5B3C         /* the companion (Medli / Makar) or the ship */
#define PLAY_STATUS0 0x5CD8
#define PLAY_STATUS1 0x5CDC
#define SAVE_PLAYER_MAP 0xE4
#define SAVE_MEMBIT 0x798
#define SAVE_CHART_SET 0x1C1
#define CHART_TABLE 0x1047A878      /* the loaded chart table (cmapdat): count, then 0x26-byte entries */
#define LOCATION_TABLE 0x1047A874   /* the loaded location table (FmapDat): count, then 8-byte entries */
#define STTS0_HIDE (0x1000u | 0x2000u | 0x4000u | 0x20000u | 0x80000u | 0x200000u) /* aiming, first person, telescope */
#define STTS0_SHIP 0x10000u
#define STTS1_CONDUCT 0x1u

enum { MODE_NONE, MODE_SEA, MODE_DUNGEON };
#define MAX_ROOMS 40
#define MAX_POINTS 48
#define MAX_CHARTS 64

static struct {
    float x, side_compact, side_expanded, bottom, opacity, faded, bg_opacity;
    int fade, background, wind, dungeons, debug;
    u32 key_toggle, key_size, key_view, key_chart;
} opt;

static struct {
    int mode, visible, sector, discovered, covered, on_ship, dungeon, floor, room, has_map, has_compass, companion;
    float x, y, z, cx, cz, island_dx, island_dz;
    s16 yaw, cyaw;
    u32 wind;
    int chart_set, chart_count;
    struct { s8 sea, save; } charts[MAX_CHARTS];
    float origin_x, origin_z, size;
    int dmap;
    unsigned long long visited_lo, visited_hi;
    int point_count;
    struct { float x, z; } points[MAX_POINTS];
    char stage[9];
} snap;

/* player choices */
static struct { int hidden, expanded, zoom, sea_view, chart, feedback; float feedback_time; } ui = {0, 0, 0, 1, 0, 0, 0};
enum { FEEDBACK_NONE, FEEDBACK_HIDDEN, FEEDBACK_EXPANDED, FEEDBACK_COMPACT };
static u32 img_feedback[4];
static const char* const feedback_art[4] = {0, "assets/hide.png", "assets/zoom.png", "assets/reduce.png"};
static u32 previous_buttons;
static hdm_fade fade = {1, 1};
static int initialized;

/* images */
static unsigned long long image_epoch;
static u32 img_frame, img_wind, img_unknown, img_face, img_arrow, img_chest, img_medli, img_makar, img_ship, img_base;
static u32 img_island, img_chart;
static int island_room = -1, island_full = -1, chart_key = -1;
static struct { int pane; float x, y, w, h; u32 image; } rooms[MAX_ROOMS];
static int room_count, rooms_dungeon = -1, rooms_floor = -1;
static float floor_min_x, floor_min_y, floor_max_x, floor_max_y;
static wwhd_game_pane all_pane;
static int all_pane_ok;

static wwhd_hud_element element;
static char name[96];
static const float pi = 3.14159265358979323846f;

static float bounded(double v, float lo, float hi, float fallback) { return v >= lo && v <= hi ? (float)v : fallback; }
static u32 key_option(const char* id, const char* fallback) {
    static char value[16];
    value[0] = 0;
    if (!wwhd_config_string(id, value, sizeof value)) { int i = 0; for (; fallback[i] && i < 15; ++i) value[i] = fallback[i]; value[i] = 0; }
    return hdm_button_bit(value);
}
static void configure(void) {
    opt.x = bounded(wwhd_config_float("x", 12), 0, 1200, 12);
    opt.bottom = bounded(wwhd_config_float("y", 20), 0, 650, 20);
    opt.side_compact = bounded(wwhd_config_float("size", 190), 96, 400, 190);
    opt.side_expanded = bounded(wwhd_config_float("size_expanded", 300), 96, 600, 300);
    opt.opacity = bounded(wwhd_config_float("opacity", 1), 0, 1, 1);
    opt.faded = bounded(wwhd_config_float("faded_opacity", .7), 0, 1, .7f);
    opt.bg_opacity = bounded(wwhd_config_float("background_opacity", .4), 0, 1, .4f);
    opt.fade = wwhd_config_bool("fade", 1);
    opt.background = wwhd_config_bool("background", 1);
    opt.wind = wwhd_config_bool("wind", 1);
    opt.dungeons = wwhd_config_bool("dungeons", 1);
    opt.debug = wwhd_config_bool("debug", 0);
    opt.key_toggle = key_option("key_toggle", "left");
    opt.key_size = key_option("key_size", "right");
    opt.key_view = key_option("key_view", "down");
    opt.key_chart = key_option("key_chart", "l3");
    ui.expanded = wwhd_config_bool("start_expanded", 0);
    initialized = 1;
}

/* ---- images ---- */
static u32 game_image(const char* spec) { return wwhd_hud_texture(WWHD_HUD_GAME, spec); }
static void drop(u32* h) { if (*h) wwhd_hud_release(*h); *h = 0; }
static void forget_rooms(void) {
    for (int i = 0; i < room_count; ++i) drop(&rooms[i].image);
    room_count = 0; rooms_dungeon = rooms_floor = -1;
}
static void forget_images(void) {
    /* after a state load the handles are gone: just forget them */
    img_frame = img_wind = img_unknown = img_face = img_arrow = img_chest = img_medli = img_makar = img_ship = img_base = 0;
    img_island = img_chart = 0; island_room = island_full = chart_key = -1;
    for (int i = 0; i < 4; ++i) img_feedback[i] = 0;
    for (int i = 0; i < room_count; ++i) rooms[i].image = 0;
    room_count = 0; rooms_dungeon = rooms_floor = -1; all_pane_ok = 0;
}
static void static_images(void) {
    if (!img_frame) img_frame = wwhd_hud_texture(WWHD_HUD_PACKAGE, "assets/frame.png");
    if (!img_face) img_face = game_image("DungeonMapLink_00/MapLinkS_00^l");
    if (!img_arrow) img_arrow = game_image("DungeonMapLink_00/MapDirectionArrow_00^t#P_Direction_00");
}
static int texture_exists(const char* spec) {
    static char key[80];
    static wwhd_game_pane info;
    int i = hdm_put(key, 0, "layout-texture:");
    for (int k = 0; spec[k] && spec[k] != '#' && i < 78; ++k) key[i++] = spec[k];
    key[i] = 0;
    return wwhd_game_pane_get(key, &info);
}
static void island_image(int room) {
    if (island_room == room) return;
    drop(&img_island);
    island_room = room;
    hdm_sector_texture(name, room, 1);
    island_full = texture_exists(name);
    if (!island_full) hdm_sector_texture(name, room, 0);
    if (island_full || texture_exists(name)) img_island = game_image(name);
}
static void chart_image(int sea) {
    int key = sea * 4 + snap.chart_set;
    if (chart_key == key) return;
    drop(&img_chart);
    chart_key = key;
    hdm_chart_texture(name, sea, snap.chart_set);
    img_chart = game_image(name);
}
/* the floor plan: the room panes of the dungeon's layout whose art is on the floor */
static void floor_rooms(int dungeon, int floor) {
    static wwhd_game_pane pane;
    static char key[64];
    char tag[3];
    if (rooms_dungeon == dungeon && rooms_floor == floor) return;
    forget_rooms();
    rooms_dungeon = dungeon; rooms_floor = floor;
    if (!hdm_floor_tag(floor, tag)) return;
    hdm_floor_layout(name, dungeon);
    int i = hdm_put(key, 0, "layout-pane:");
    i = hdm_put(key, i, name);
    hdm_put(key, i, "/N_All_00");
    all_pane_ok = wwhd_game_pane_get(key, &all_pane);
    floor_min_x = floor_min_y = 1e9f; floor_max_x = floor_max_y = -1e9f;
    for (int p = 0; p < MAX_ROOMS; ++p) {
        hdm_room_pane_key(key, dungeon, p);
        if (!wwhd_game_pane_get(key, &pane) || !(pane.flags & 2)) continue;
        pane.texture[31] = 0;
        if (!hdm_texture_on_floor(pane.texture, tag)) continue;
        rooms[room_count].pane = p;
        rooms[room_count].x = pane.x; rooms[room_count].y = pane.y;
        rooms[room_count].w = pane.w; rooms[room_count].h = pane.h;
        hdm_room_texture(name, dungeon, pane.texture, p);
        rooms[room_count].image = game_image(name);
        if (pane.x - pane.w / 2 < floor_min_x) floor_min_x = pane.x - pane.w / 2;
        if (pane.x + pane.w / 2 > floor_max_x) floor_max_x = pane.x + pane.w / 2;
        if (pane.y - pane.h / 2 < floor_min_y) floor_min_y = pane.y - pane.h / 2;
        if (pane.y + pane.h / 2 > floor_max_y) floor_max_y = pane.y + pane.h / 2;
        if (++room_count == MAX_ROOMS) break;
    }
}
/* world X/Z -> the floor layout's root coordinates: the stage's DMAP entry gives the world point at the
 * centre of the layout's map area (N_All_00) and the world size spanned by its width */
static void world_to_layout(float x, float z, float* lx, float* ly) {
    float k = snap.size > 0 && all_pane_ok ? all_pane.w / snap.size : 0;
    float cx = all_pane_ok ? all_pane.x : 0, cy = all_pane_ok ? all_pane.y : 0;
    *lx = cx + (x - snap.origin_x) * k;
    *ly = cy - (z - snap.origin_z) * k;
}

/* ---- drawing ---- */
static void image(u32 list, u32 handle, float x, float y, float w, float h, float rotation, u32 rgb, float alpha) {
    if (!handle || alpha <= 0) return;
    element = (wwhd_hud_element){0};
    element.kind = WWHD_HUD_IMAGE; element.anchor = WWHD_HUD_TOP_LEFT;
    element.x = x; element.y = y; element.w = w; element.h = h; element.rotation = rotation;
    element.thickness = 1; element.u1 = element.v1 = 1; element.image = handle;
    element.rgba = (rgb << 8) | (u32)(alpha * 255.0f + .5f);
    wwhd_hud_emit(list, &element);
}
static void rect(u32 list, float x, float y, float w, float h, u32 rgb, float alpha) {
    if (alpha <= 0) return;
    element = (wwhd_hud_element){0};
    element.kind = WWHD_HUD_RECT; element.anchor = WWHD_HUD_TOP_LEFT;
    element.x = x; element.y = y; element.w = w; element.h = h; element.thickness = 1; element.u1 = element.v1 = 1;
    element.rgba = (rgb << 8) | (u32)(alpha * 255.0f + .5f);
    wwhd_hud_emit(list, &element);
}
static void clip(u32 list, int push, float x, float y, float w, float h) {
    element = (wwhd_hud_element){0};
    element.kind = push ? WWHD_HUD_CLIP_PUSH : WWHD_HUD_CLIP_POP;
    element.anchor = WWHD_HUD_TOP_LEFT;
    element.x = x; element.y = y; element.w = w; element.h = h; element.thickness = 1;
    wwhd_hud_clip(list, &element);
}
static float yaw_rotation(s16 yaw) { return pi - (float)yaw * (2 * pi / 65536.0f); } /* art points up, yaw 0 faces +Z (down) */
static void marker(u32 list, u32 handle, float px, float py, float size, float rotation, float alpha) {
    image(list, handle, px - size / 2, py - size / 2, size, size, rotation, 0xffffff, alpha);
}
/* the game's marker (DungeonMapLink_00): the face (64 x 64 at 0.7) and a 34 x 22 heading arrow 29 units
 * from its centre, turned with Link; sizes in units of the face's 44.8 */
static float sin_approx(float x) {
    while (x > pi) x -= 2 * pi;
    while (x < -pi) x += 2 * pi;
    float y = x * (1.27323954f - 0.405284735f * (x < 0 ? -x : x));
    return y * (0.775f + 0.225f * (y < 0 ? -y : y));
}
static void heading_marker(u32 list, u32 icon, float icon_size, float px, float py, float side, s16 yaw, float alpha) {
    float unit = side * .12f / 44.8f, r = yaw_rotation(yaw), d = 29 * unit;
    float ax = px + d * sin_approx(r), ay = py - d * sin_approx(r + pi / 2);
    image(list, img_arrow, ax - 17 * unit, ay - 11 * unit, 34 * unit, 22 * unit, r, 0xffffff, alpha);
    marker(list, icon, px, py, icon_size > 0 ? icon_size : 44.8f * unit, 0, alpha);
}
static void link_marker(u32 list, float px, float py, float side, float alpha) {
    heading_marker(list, img_face, 0, px, py, side, snap.yaw, alpha);
}
static void frame(u32 list, float x, float y, float side, float a) {
    image(list, img_frame, x, y, side, side, 0, 0xffffff, a);
}

static void draw_sea(u32 list, float x, float y, float side, float a) {
    float inset = side * 6.0f / 256.0f, inner = side - 2 * inset;
    if (!snap.discovered || !snap.covered) {
        if (snap.discovered) return;  /* outside the island art: the game's compasses only */
        /* undiscovered sector: the hint only, like the game's "?" */
        if (!img_unknown) img_unknown = wwhd_hud_texture(WWHD_HUD_PACKAGE, "assets/unknown.png");
        float s = side * .32f;
        image(list, img_unknown, x + inset, y + side - s - inset, s, s, 0, 0xffffff, a * .6f);
        return;
    }
    island_image(snap.sector);
    if (!img_island) return;
    if (!img_base) img_base = game_image("BtnMapBalloon_00/MapArea_00^t#P_MapBase_00");
    if (opt.background) rect(list, x + inset, y + inset, inner, inner, 0x000000, opt.bg_opacity * a / (opt.opacity > 0 ? opt.opacity : 1));
    clip(list, 1, x + inset, y + inset, inner, inner);
    image(list, img_base, x + inset - inner * .1f, y + inset - inner * .1f, inner * 1.2f, inner * 1.2f, 0, 0xffffff, a);
    image(list, img_island, x + inset, y + inset, inner, inner, 0, 0xffffff, a);
    float u, v;
    hdm_island_uv(snap.sector, snap.island_dx, snap.island_dz, snap.x, snap.z, &u, &v);
    if (snap.on_ship) {
        if (!img_ship) img_ship = game_image("Map_00/MapShip_00^t");
        heading_marker(list, img_ship, side * .13f, x + inset + u * inner, y + inset + v * inner, side, snap.yaw, a);
    } else {
        link_marker(list, x + inset + u * inner, y + inset + v * inner, side, a);
    }
    clip(list, 0, 0, 0, 0, 0);
    frame(list, x, y, side, a);
    if (opt.wind) {
        if (!img_wind) img_wind = wwhd_hud_texture(WWHD_HUD_PACKAGE, "assets/wind.png");
        /* the game's angle points where the wind comes from; the arrow shows where it blows */
        s16 toward = (s16)(u16)(snap.wind + 0x8000u);
        float s = side * .2f;
        image(list, img_wind, x + side - s - inset * .5f, y + inset * .5f, s, s, yaw_rotation(toward), 0xffffff, a);
    }
}
static void draw_chart(u32 list, float x, float y, float side, float a) {
    if (!snap.chart_count) return;
    if (ui.chart >= snap.chart_count) ui.chart = 0;
    chart_image(snap.charts[ui.chart].sea);
    float inset = side * 6.0f / 256.0f, inner = side - 2 * inset;
    if (opt.background) rect(list, x + inset, y + inset, inner, inner, 0x000000, opt.bg_opacity * a / (opt.opacity > 0 ? opt.opacity : 1));
    image(list, img_chart, x + inset, y + inset, inner, inner, 0, 0xffffff, a);
    frame(list, x, y, side, a);
}
static void draw_dungeon(u32 list, float x, float y, float side, float a) {
    floor_rooms(snap.dungeon, snap.floor);
    if (!room_count) return;
    float inset = side * 6.0f / 256.0f, inner = side - 2 * inset;
    float link_x, link_y;
    world_to_layout(snap.x, snap.z, &link_x, &link_y);
    hdm_view view;
    if (ui.zoom) {
        float span = (floor_max_x - floor_min_x > floor_max_y - floor_min_y ? floor_max_x - floor_min_x : floor_max_y - floor_min_y) * .45f;
        view = hdm_fit(link_x - span / 2, link_y - span / 2, link_x + span / 2, link_y + span / 2, inner, 0);
    } else {
        view = hdm_fit(floor_min_x, floor_min_y, floor_max_x, floor_max_y, inner, .04f);
    }
    if (opt.background) rect(list, x + inset, y + inset, inner, inner, 0x000000, opt.bg_opacity * a / (opt.opacity > 0 ? opt.opacity : 1));
    clip(list, 1, x + inset, y + inset, inner, inner);
    for (int i = 0; i < room_count; ++i) {
        int p = rooms[i].pane;
        int visited = p < 64 && ((p < 32 ? snap.visited_lo >> p : snap.visited_hi >> (p - 32)) & 1);
        u32 rgb = p == snap.room ? 0xffffff : visited ? 0xd8d8d8 : 0x8c8c8c;
        float px, py;
        hdm_to_panel(&view, inner, rooms[i].x - rooms[i].w / 2, rooms[i].y + rooms[i].h / 2, &px, &py);
        image(list, rooms[i].image, x + inset + px, y + inset + py, rooms[i].w * view.scale, rooms[i].h * view.scale, 0, rgb,
              visited || p == snap.room ? a : a * .75f);
    }
    if (snap.has_compass) {
        if (!img_chest) img_chest = game_image("DungeonMapTreasure_00/MapTreasure_00^t");
        for (int i = 0; i < snap.point_count; ++i) {
            float lx, ly, px, py;
            world_to_layout(snap.points[i].x, snap.points[i].z, &lx, &ly);
            hdm_to_panel(&view, inner, lx, ly, &px, &py);
            image(list, img_chest, x + inset + px - side * .035f, y + inset + py - side * .03f, side * .07f, side * .06f, 0, 0xffd860, a);
        }
    }
    if (snap.companion) {
        u32* img = snap.dungeon == 4 ? &img_medli : &img_makar;
        if (!*img) *img = game_image(snap.dungeon == 4 ? "DungeonMapMedley_00/MapMedleyS_00^q" : "DungeonMapMakore_00/MapMakoreS_00^q");
        float lx, ly, px, py;
        world_to_layout(snap.cx, snap.cz, &lx, &ly);
        hdm_to_panel(&view, inner, lx, ly, &px, &py);
        marker(list, *img, x + inset + px, y + inset + py, side * .1f, 0, a);
    }
    {
        float px, py;
        hdm_to_panel(&view, inner, link_x, link_y, &px, &py);
        link_marker(list, x + inset + px, y + inset + py, side, a);
    }
    clip(list, 0, 0, 0, 0, 0);
    frame(list, x, y, side, a);
}

static unsigned long long captured_step = ~0ull;
static void draw(u32 list) {
    unsigned long long epoch = wwhd_hud_epoch();
    if (epoch != image_epoch) { image_epoch = epoch; forget_images(); }
    /* hidden while the game is paused or a menu is open: Link's step did not run for this logic step */
    if (opt.debug && (wwhd_logic_step() % 60) == 0) {
        wwhd_log_int("draw.visible", snap.visible); wwhd_log_int("draw.mode", snap.mode);
        wwhd_log_int("draw.sector", snap.sector); wwhd_log_int("draw.discovered", snap.discovered);
        wwhd_log_hex("draw.step", (u32)wwhd_logic_step()); wwhd_log_hex("draw.captured", (u32)captured_step);
        wwhd_log_int("draw.paused", game_paused());
        wwhd_log_float("sea.x", snap.x); wwhd_log_float("sea.z", snap.z);
        wwhd_log_float("sea.island_dx", snap.island_dx); wwhd_log_float("sea.island_dz", snap.island_dz);
        wwhd_log_int("sea.covered", snap.covered);
    }
    if (!snap.visible || game_paused() || wwhd_logic_step() > captured_step + 1) return;
    if (ui.feedback && ui.feedback_time > 0) {
        /* the shortcut's picture for a moment after a press (Minimap HD's icons) */
        if (!img_feedback[ui.feedback]) img_feedback[ui.feedback] = wwhd_hud_texture(WWHD_HUD_PACKAGE, feedback_art[ui.feedback]);
        float t = ui.feedback_time > .3f ? 1 : ui.feedback_time / .3f, s = 56;
        image(list, img_feedback[ui.feedback], opt.x, 720 - opt.bottom - s, s, s, 0, 0xffffff, t * (opt.opacity > .3f ? opt.opacity : .3f));
    }
    if (ui.hidden) return;  /* Link's step ran in the step just finished */
    float rest = opt.opacity, a = opt.fade ? hdm_fade_opacity(&fade, rest, opt.faded) : rest;
    if (a <= 0) return;
    static_images();
    if (snap.mode == MODE_SEA) {
        float side = snap.on_ship ? opt.side_compact : (ui.expanded ? opt.side_expanded : opt.side_compact);
        if (snap.on_ship && ui.sea_view == 0) return;
        float x = opt.x, y = 720 - side - opt.bottom;
        draw_sea(list, x, y, side, a);
        if (snap.on_ship && ui.sea_view == 2 && snap.discovered) draw_chart(list, x + side + 8, y, side, a);
    } else if (snap.mode == MODE_DUNGEON) {
        float side = ui.expanded ? opt.side_expanded : opt.side_compact;
        draw_dungeon(list, opt.x, 720 - side - opt.bottom, side, a);
    }
}

/* ---- per-step capture (Link's own step, so it doesn't run in menus or while paused) ---- */
static void capture_chests(u32 membit, const u8* records, s32 count, int filter_room, int stage_kind) {
    if (!records || count < 0 || count > 128) return;
    for (s32 i = 0; i < count && snap.point_count < MAX_POINTS; ++i) {
        const u8* r = records + 32 * i;
        u32 params = ((u32)r[8] << 24) | ((u32)r[9] << 16) | ((u32)r[10] << 8) | r[11];
        float x = *(const f32*)(r + 12), y = *(const f32*)(r + 16), z = *(const f32*)(r + 20);
        u32 rooms = ((u32)r[24] << 8) | r[25];
        u32 number = (params >> 7) & 31, type = params & 127;
        if (number == 15 || number == 16) continue;
        if (filter_room && (int)(rooms & 63) != snap.room) continue;
        int opened = (type == 7 || type == 8) ? stage_chest_open(stage_kind, (s32)number) : chest_open(membit, (s32)number);
        if (opened || room_floor((s32)(rooms & 63), y) != snap.floor) continue;
        snap.points[snap.point_count].x = x; snap.points[snap.point_count].z = z; ++snap.point_count;
    }
}
static void read_stage(u8* play) {
    u8* stage = play + PLAY_STAGE;
    u32 dmap = *(u32*)(stage + 0x74);
    snap.dmap = 0;
    if (dmap) {
        u32 entries = *(u32*)(dmap + 4);
        if (entries) {
            snap.origin_x = *(f32*)entries; snap.origin_z = *(f32*)(entries + 4); snap.size = *(f32*)(entries + 8);
            snap.dmap = hdm_finite(snap.size) && snap.size > 0;
        }
    }
}
static float floor_offset(u8* play) {
    u32 info = *(u32*)(play + PLAY_STAGE + 0x48);
    u32 dmap = *(u32*)(play + PLAY_STAGE + 0x74);
    if (!info || !dmap || !*(u32*)(dmap + 4)) return 0;
    u32 kind = (*(u32*)(info + 0xC) >> 16) & 7;
    return (kind == 3 || kind == 6) ? *(f32*)(*(u32*)(dmap + 4) + 12) : 0;
}
static void read_charts(u32 save) {
    snap.chart_count = 0;
    u32 table = *(u32*)&WWHD_GAME_DATA(CHART_TABLE, u8);
    if (!table) return;
    s32 count = *(s32*)table;
    if (count <= 0 || count > 64) return;
    for (s32 i = 0; i < count && snap.chart_count < MAX_CHARTS; ++i) {
        const u8* e = (const u8*)(table + 4 + 0x26 * i);
        s8 sea = (s8)e[0], save_no = (s8)e[1];
        u8 type = e[3];
        if (sea < 1 || sea > 49 || save_no < 1) continue;          /* sector charts only: no overview charts */
        if (!chart_owned(save + SAVE_PLAYER_MAP, save_no - 1)) continue;
        if (type == 0 && !chart_open(save + SAVE_PLAYER_MAP, save_no - 1)) continue;  /* Triforce charts once deciphered */
        snap.charts[snap.chart_count].sea = sea; snap.charts[snap.chart_count].save = save_no; ++snap.chart_count;
    }
}
static void handle_input(void) {
    wwhd_input_state in;
    wwhd_input_read(&in);
    u32 b = in.buttons;
    if (ui.feedback_time > 0) ui.feedback_time -= (float)wwhd_logic_dt();
    if (hdm_pressed(b, previous_buttons, opt.key_toggle)) {
        ui.hidden = !ui.hidden;
        ui.feedback = ui.hidden ? FEEDBACK_HIDDEN : (ui.expanded ? FEEDBACK_EXPANDED : FEEDBACK_COMPACT);
        ui.feedback_time = ui.hidden ? 1.5f : 0;
    }
    if (!ui.hidden) {
        if (hdm_pressed(b, previous_buttons, opt.key_size) && !(snap.mode == MODE_SEA && snap.on_ship)) {
            ui.expanded = !ui.expanded;
            ui.feedback = ui.expanded ? FEEDBACK_EXPANDED : FEEDBACK_COMPACT;
            ui.feedback_time = 1.0f;
        }
        if (hdm_pressed(b, previous_buttons, opt.key_view)) {
            if (snap.mode == MODE_DUNGEON) ui.zoom = !ui.zoom;
            else if (snap.mode == MODE_SEA && snap.on_ship) ui.sea_view = hdm_next_sea_view(ui.sea_view, snap.chart_count);
        }
        if (hdm_pressed(b, previous_buttons, opt.key_chart) && snap.mode == MODE_SEA && snap.on_ship && ui.sea_view == 2 && snap.chart_count)
            ui.chart = (ui.chart + 1) % snap.chart_count;
    }
    previous_buttons = b;
    hdm_fade_step(&fade, hdm_moving(in.lx, in.ly, in.rx, in.ry), (float)wwhd_logic_dt());
}

WWHD_HOOK_RETURN(WWHD_ADDR_daPy_Execute, capture, (fopAc_ac_c* actor)) {
    if (!initialized) configure();
    wwhd_hud_register(draw, WWHD_HUD_TV);
    u8* play = wwhd_play_get();
    if (!play || !actor || (u32)actor != *(u32*)(play + WWHD_PLAY_PLAYER_OFFSET)) return;
    captured_step = wwhd_logic_step();
    snap.visible = 0;
    if (play[WWHD_PLAY_ENABLE_NEXT_STAGE_OFFSET] || play[WWHD_PLAY_EVENT_RUNNING_OFFSET]) return;
    u32 status0 = *(u32*)(play + PLAY_STATUS0), status1 = *(u32*)(play + PLAY_STATUS1);
    handle_input();
    if ((status0 & STTS0_HIDE) || (status1 & STTS1_CONDUCT)) return;
    for (int i = 0, ended = 0; i < 8; ++i) {
        char ch = ended ? 0 : (char)play[WWHD_PLAY_START_STAGE_NAME_OFFSET + i];
        if (!ch) ended = 1;
        snap.stage[i] = ch;
    }
    snap.stage[8] = 0;
    u32 save = *(u32*)&WWHD_GAME_DATA(WWHD_ADDR_dComIfG_save_info_pointer, u8);
    if (!save) return;
    snap.x = actor->current.pos.x; snap.y = actor->current.pos.y; snap.z = actor->current.pos.z;
    snap.yaw = actor->shape_angle.y;
    snap.room = actor->current.roomNo;
    snap.on_ship = (status0 & STTS0_SHIP) != 0;
    if (hdm_same(snap.stage, "sea")) {
        snap.mode = MODE_SEA;
        snap.sector = hdm_sea_room(snap.x, snap.z);
        if (!snap.sector) return;
        snap.discovered = sector_discovered(save + SAVE_PLAYER_MAP, snap.sector - 1) != 0;
        const u8* location = hdm_location(*(const u8**)&WWHD_GAME_DATA(LOCATION_TABLE, u8), snap.sector);
        snap.covered = 0;
        if (location) {
            float u, v;
            snap.island_dx = hdm_s16(location + 2); snap.island_dz = hdm_s16(location + 4);
            hdm_island_uv(snap.sector, snap.island_dx, snap.island_dz, snap.x, snap.z, &u, &v);
            snap.covered = hdm_inside(u, v);
        }
        snap.wind = wind_angle();
        snap.chart_set = *(u8*)(save + SAVE_CHART_SET) & 3;
        if (snap.on_ship) read_charts(save); else snap.chart_count = 0;
        snap.visible = 1;
        return;
    }
    snap.dungeon = hdm_dungeon_of(snap.stage);
    if (snap.dungeon < 0 || !opt.dungeons || snap.room < 0 || snap.room >= 64) { snap.mode = MODE_NONE; return; }
    u32 membit = save + SAVE_MEMBIT;
    int forsaken = snap.dungeon == 0;
    snap.has_map = forsaken ? stage_map(2) != 0 : dungeon_item(membit, 0) != 0;
    if (!snap.has_map) { snap.mode = MODE_NONE; return; }
    snap.has_compass = forsaken ? stage_compass(2) != 0 : dungeon_item(membit, 1) != 0;
    read_stage(play);
    if (!snap.dmap) { snap.mode = MODE_NONE; return; }
    snap.floor = stage_floor(0, snap.y + floor_offset(play));
    snap.visited_lo = snap.visited_hi = 0;
    for (int r = 0; r < 64; ++r)
        if (room_visited(membit, r)) { if (r < 32) snap.visited_lo |= 1ull << r; else snap.visited_hi |= 1ull << (r - 32); }
    snap.point_count = 0;
    if (snap.has_compass) {
        u8* table = stage_treasures();
        if (table) capture_chests(membit, table + 4, *(s32*)table, 1, 2);
        u8* room = room_data((u32)play + 0x51cc, snap.room);
        if (room) { u8* t = *(u8**)(room + 0x40); if (t) capture_chests(membit, *(u8**)(t + 4), *(s32*)t, 0, 2); }
    }
    snap.companion = 0;
    if (snap.dungeon == 4 || snap.dungeon == 5) {
        fopAc_ac_c* partner = *(fopAc_ac_c**)(play + PLAY_PARTNER);
        if (partner && partner != actor) {
            snap.cx = partner->current.pos.x; snap.cz = partner->current.pos.z; snap.cyaw = partner->shape_angle.y;
            snap.companion = room_floor(partner->current.roomNo, partner->current.pos.y + floor_offset(play)) == snap.floor;
        }
    }
    if (opt.debug && (wwhd_logic_step() % 30) == 0) {
        float lx, ly;
        world_to_layout(snap.x, snap.z, &lx, &ly);
        wwhd_log_int("room", snap.room); wwhd_log_int("floor", snap.floor);
        wwhd_log_float("x", snap.x); wwhd_log_float("z", snap.z);
        wwhd_log_float("dmap.x", snap.origin_x); wwhd_log_float("dmap.z", snap.origin_z); wwhd_log_float("dmap.size", snap.size);
        wwhd_log_float("layout.x", lx); wwhd_log_float("layout.y", ly);
        for (int i = 0; i < room_count; ++i) if (rooms[i].pane == snap.room) {
            wwhd_log_float("pane.x", rooms[i].x); wwhd_log_float("pane.y", rooms[i].y);
            wwhd_log_float("pane.w", rooms[i].w); wwhd_log_float("pane.h", rooms[i].h);
        }
    }
    snap.mode = MODE_DUNGEON;
    snap.visible = 1;
}
