/* Leviathan: rarely, at night far out on the Great Sea, the wind dies and fog rolls in for 30 seconds, then a Gyorg
 * eight times the size of any other surfaces behind Link and circles him like a shark for three minutes, ramming the
 * boat now and then. Making land, coming near an island or holding out ends it; killing it pays 200 rupees and fills
 * an empty bottle with blue potion.
 *
 * Runs once per logic step, after the actors' execute (a return hook on fpcEx_Handler). The Leviathan is the game's own
 * Gyorg (daGy_c), made with fopAcM_create in Link's room and process layer as the child of a quiet Gyorg controller
 * (daGyCtrl, as Haunted sea makes it), then held at 8x. The game makes Gyorgs only once Link has the Boomerang, so it
 * only comes then. Offsets are the HD game's (USA and EU). */
#include "wwhd_guest.h"
#include "wwhd/functions.h"
#include "wwhd/data.h"
#include "wwhd/actor.h"
#include "wwhd/link.h"
#include "wwhd/objects.h"
#include "wwhd/save.h"

/* ---- the game ---- */
WWHD_GAME_FUNC(0x025D5834, u32, lv_create, (u32 proc, u32 param, cXyz* pos, s32 room, csXyz* angle, cXyz* scale, s32 subtype, u32 create_func));
WWHD_GAME_FUNC(0x025D5A20, u32, lv_create_child, (u32 proc, u32 parent, u32 param, cXyz* pos, s32 room, csXyz* angle, cXyz* scale, s32 subtype, u32 create_func));
WWHD_GAME_FUNC(0x025DE50C, u8*, lv_search, (u32 id));          /* fpcEx_SearchByID: the process while it exists */
WWHD_GAME_FUNC(0x025D57E4, s32, lv_delete, (u32 id));          /* fopAcM_delete by id */
WWHD_GAME_FUNC(0x025DED64, u32, lv_current_layer, (void));      /* fpcLy_CurrentLayer */
WWHD_GAME_FUNC(0x025DEAB4, void, lv_set_layer, (u32 layer));     /* fpcLy_SetCurrentLayer */
WWHD_GAME_FUNC(0x0257E594, void, lv_windpower, (f32 power));     /* dKyw_custom_windpower: above 0 overrides the wind, 0 hands it back */
WWHD_GAME_FUNC(0x0255671C, void, lv_fog, (f32 start, f32 end, f32 ratio));  /* dKy_fog_startendz_set */
WWHD_GAME_FUNC(0x02169B68, void, lv_ram, (u8* gyorg));            /* daGy_c::modeAttackInit: breach and ram the ship */
WWHD_GAME_FUNC(0x0216810C, void, lv_flinch, (u8* gyorg));         /* daGy_c's hurt reaction, as after a hit it survives */
WWHD_GAME_FUNC(0x0254DA38, void, lv_item_get, (u32 item));        /* execItemGet */
WWHD_GAME_FUNC(0x0254F3BC, void, lv_fill_blue, (void));           /* item_func_blue_bottle: the first empty bottle */
WWHD_GAME_FUNC(0x025E1988, void, lv_sound, (u32 id));             /* mDoAud_seStart */

#define NONE 0xFFFFFFFFu
#define PI 3.14159265358979323846
#define ITEM_BOOMERANG 0x2D
#define ITEM_WIND_WAKER 0x22
#define ITEM_SAIL 0x78
#define ITEM_SILVER_RUPEE 0x0F      /* 200 */
#define ITEM_EMPTY_BOTTLE 0x50
#define SOUND_RUPEE 0x836
#define SOUND_PICKUP 0x827
#define PLAY_LINK 0x5B34            /* dComIfG_play_c: Link */
#define SAVE_EVENT 0x644            /* the save's dSv_event_c */
#define PROC_NAME 8                 /* base_process_class: process name */
/* daGy_c */
#define GY_HEALTH 0x3A1             /* s8, 1 for a Gyorg */
#define GY_CHILD 0x3C8              /* its index among its controller's children */
#define GY_MODE 0x3CC               /* 1 circle, 2 ram, 7 hurt, 8 frozen away, 9 dying */
#define GY_LAST_MODE 0x3D0
#define GY_CTRL 0x3D4               /* its controller */
#define GY_BODY_R 0x9B4             /* its body collider's radius, stored each execute */
#define GY_FROZEN 0xE04             /* 9 / 0xB: hit by an ice / light arrow */
#define CTRL_CLEAR 0x42E            /* daGyCtrl: per child, the clear-line flag; set, its Gyorg circles the ship */

static u32 word(u8* p, u32 o) { return *(volatile u32*)(p + o); }
static void set_word(u8* p, u32 o, u32 v) { *(volatile u32*)(p + o) = v; }
static f32 flt(u8* p, u32 o) { return *(volatile f32*)(p + o); }
static void set_flt(u8* p, u32 o, f32 v) { *(volatile f32*)(p + o) = v; }
static cXyz pos_of(u8* a) { return ((fopAc_ac_c*)a)->current.pos; }
static u32 bits(f32 f) { union { f32 f; u32 u; } v = {f}; return v.u; }
static int in_mem2(u32 a) { return a >= 0x10000000 && a < 0x50000000; }

/* ---- freestanding math and randomness (no libm in guest mods) ---- */
static double wrap(double x) {
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    return x;
}
static double lv_sin(double x) {
    x = wrap(x);
    if (x > PI / 2) x = PI - x;
    if (x < -PI / 2) x = -PI - x;
    const double q = x * x;
    return x * (1 + q * (-1.0 / 6 + q * (1.0 / 120 + q * (-1.0 / 5040 + q * (1.0 / 362880)))));
}
static double lv_cos(double x) { return lv_sin(x + PI / 2); }
static double lv_sqrt(double x) {
    if (x <= 0) return 0;
    double r = x > 1 ? x / 2 : 1;
    for (int i = 0; i < 30; i++) r = (r + x / r) / 2;
    return r;
}
static double dist2d(cXyz a, cXyz b) { return lv_sqrt((double)(a.x - b.x) * (a.x - b.x) + (double)(a.z - b.z) * (a.z - b.z)); }
static double fmin_(double a, double b) { return a < b ? a : b; }
static double fmax_(double a, double b) { return a > b ? a : b; }
static double round_(double x) { return x < 0 ? -(double)(s32)(-x + 0.5) : (double)(s32)(x + 0.5); }  /* (|x| < 2^31 here) */

static u32 rng;
static u32 rnd(void) {
    rng ^= rng << 13, rng ^= rng >> 17, rng ^= rng << 5;
    return rng;
}
static double rnd01(void) { return (rnd() >> 8) / 16777216.0; }

/* ---- options ---- */
/* How often: the chance each minute far out at sea at night (x0.4, x1, x3), or Always (far out at sea, day and night,
 * without the signs) */
static int often = 1;
static const double kScale[] = {0.4, 1, 3};
static int choice(const char* id, const char* const* names, int count, int fallback) {
    char value[32];
    if (!wwhd_config_string(id, value, sizeof value)) return fallback;
    for (int i = 0; i < count; i++) {
        const char* a = value;
        const char* b = names[i];
        while (*a && *a == *b) a++, b++;
        if (!*a && !*b) return i;
    }
    return fallback;
}
static int always(void) { return often == 3; }

static u8* save_info(void) { return (u8*)WWHD_GAME_DATA(WWHD_ADDR_save_info_pointer, u32); }
static int has_item(u8 item) {
    const u8* inventory = save_info() + WWHD_OFFSET_save_inventory;
    for (int i = 0; i < 21; i++)
        if (inventory[i] == item) return 1;
    return 0;
}

/* ---- the Leviathan ---- */
#define SIZE 8.f
#define HEALTH 48                   /* an arrow, the hookshot or the Boomerang take 2, a bomb or a light arrow 4 */
static const u32 kGyCtrlParam = 0x7F00FF01;  /* type 1, no Gyorgs of its own, waits on switch 0x7F (never set at sea) */
#define GYCTRL_PROC 0xE5
#define GYORG_PROC 0xE4
static u32 ctrl = NONE, beast = NONE;
static u8* beast_at;                /* for the hooks, which look it up by address */
static int health = HEALTH, slain;
static double now, next_make, rammed, slain_at = -1, last_hit = -1;

static cXyz spawn_pos;
static csXyz spawn_angle;
/* made in Link's room and his process layer (base_process_class +0x2C), as an actor's own spawns are */
static u32 make(u8* link, u16 proc, u32 param, double dist, s32 dy, u32 parent) {
    const double heading = ((fopAc_ac_c*)link)->shape_angle.y * PI / 32768;
    const double a = heading + PI + (rnd01() - 0.5) * 1.6;  /* behind him */
    const cXyz at = pos_of(link);
    spawn_pos.x = (f32)(at.x + lv_sin(a) * dist), spawn_pos.y = (f32)dy, spawn_pos.z = (f32)(at.z + lv_cos(a) * dist);
    spawn_angle.x = 0, spawn_angle.y = (s16)(s32)(wrap(a + PI) * 32768 / PI), spawn_angle.z = 0;
    const u32 layer = lv_current_layer();
    lv_set_layer(word(link, 0x2C));
    const s32 room = ((fopAc_ac_c*)link)->current.roomNo;
    const u32 id = parent != NONE ? lv_create_child(proc, parent, param, &spawn_pos, room, &spawn_angle, 0, -1, 0)
                                  : lv_create(proc, param, &spawn_pos, room, &spawn_angle, 0, -1, 0);
    lv_set_layer(layer);
    return id;
}
static void remove_all(void) {
    if (beast != NONE && lv_search(beast)) lv_delete(beast);
    if (ctrl != NONE && lv_search(ctrl)) lv_delete(ctrl);
    beast = ctrl = NONE, beast_at = 0;
}
/* a kill's loot: a silver rupee through the game's own item get (the wallet counts it up to what it holds), and blue
 * potion in the first empty bottle (inventory slots 14-17) */
static void loot(void) {
    lv_item_get(ITEM_SILVER_RUPEE);
    lv_sound(SOUND_RUPEE);
    const u8* inventory = save_info() + WWHD_OFFSET_save_inventory;
    for (int i = 14; i < 18; i++)
        if (inventory[i] == ITEM_EMPTY_BOTTLE) {
            lv_fill_blue();
            lv_sound(SOUND_PICKUP);
            break;
        }
}

/* ---- the spell ---- */
static double until;                /* the spell's end (0: none) */
static double cool;                 /* no spell before this */
static double out_since, file_at = -1, last_roll, wet;
static int warning;                 /* the signs are showing */
static double warn_end;
#define WARNING 30.0                /* seconds of signs */
#define LENGTH 180.0
#define QUIET 720.0                 /* seconds of quiet after a spell */

static void finish(const char* why) {
    wwhd_log(why);
    until = 0;
    if (!always()) cool = now + QUIET;
    remove_all();
    slain = 0, slain_at = -1;
}

static void direct(u8* play, u8* link, int night, int water, int at_sea, int settled) {
    const cXyz at = pos_of(link);
    /* the sea is a 7x7 grid of 100000-unit squares, each island near its square's centre */
    const double dx = at.x - round_(at.x / 1e5) * 1e5, dz = at.z - round_(at.z / 1e5) * 1e5;
    const double isle = lv_sqrt(dx * dx + dz * dz);
    if (water) wet = now;  /* knocked off the boat he is neither riding nor swimming for a moment */
    const int ashore = !at_sea || now - wet > 3, cutscene = play[WWHD_PLAY_EVENT_RUNNING_OFFSET] != 0;
    if (ashore) out_since = now;
    if (file_at < 0) file_at = now, cool = fmax_(cool, now + 240);  /* a file loaded: no spell in its first 4 minutes */

    if (until > 0 && !(slain_at >= 0)) {  /* (a slain one's death plays out first) */
        if (ashore || isle < 3500) finish(ashore ? "leviathan: ends, Link is off the water" : "leviathan: ends near an island");
        else if (now >= until) finish("leviathan: ends, it loses interest");
    }
    if (warning) {
        if (ashore || isle < 3500) {
            wwhd_log("leviathan: the signs fade, Link made land");
            warning = 0, lv_windpower(0);
        } else if (now >= warn_end) {
            warning = 0, lv_windpower(0);
            until = now + LENGTH, next_make = now;
            wwhd_log("leviathan: begins");
        } else {  /* the wind dies away */
            static double next;
            if (now >= next) {
                next = now + 2;
                lv_windpower((f32)fmax_(0.02, 0.3 * (1 - (now - (warn_end - WARNING)) / WARNING)));
            }
        }
    }
    if (always() || now - last_roll < 60 || !settled || cutscene) return;
    last_roll = now;
    if (!has_item(ITEM_SAIL) || !has_item(ITEM_WIND_WAKER) || !has_item(ITEM_BOOMERANG)) return;
    if (now < cool || warning || until > 0 || ashore || isle < 6000 || !night) return;
    const double more = fmin_(3, 1 + (now - out_since) / 600);  /* the longer he stays out, up to x3 */
    if (rnd01() < 0.018 * more * kScale[often]) {
        warning = 1, warn_end = now + WARNING;
        wwhd_log("leviathan: the signs show");
    }
}

/* ---- the step ---- */
static u8* last_link;
static double settle;
static f32 fog;
static int fogged;

WWHD_HOOK_RETURN(WWHD_ADDR_fpcEx_Handler_025DE788, leviathan_step, (u32 queue)) {
    (void)queue;
    /* once per logic step: in true 60 (0.5.x ports) the executor also runs between steps */
    static unsigned long long last_step;
    const unsigned long long step = wwhd_logic_step();
    if (step == last_step) return;
    last_step = step;
    const double dt = wwhd_logic_dt();
    now += dt > 0 && dt < 0.2 ? dt : 1.0 / 30;
    u8* play = wwhd_play_get();
    beast_at = 0;
    if (!play) return;
    u8* link = (u8*)word(play, PLAY_LINK);
    const char* stage = (const char*)play + WWHD_PLAY_START_STAGE_NAME_OFFSET;
    const int title = !stage[0] || (stage[0] == 's' && stage[1] == 'e' && stage[2] == 'a' && stage[3] == '_' && stage[4] == 'T') ||
                      (stage[0] == 'N' && stage[1] == 'a' && stage[2] == 'm' && stage[3] == 'e' && !stage[4]) ||
                      (stage[0] == 'E' && stage[1] == 'N' && stage[2] == 'D' && stage[3] == 'u' && stage[4] == 'm' && stage[5] == 'i');
    if (!link || title) {
        file_at = -1;
        return;
    }
    if (!rng) {  /* seeded once, from the step count and where Link stands */
        const cXyz at = pos_of(link);
        rng = (u32)wwhd_logic_step() * 2654435761u ^ bits(at.x) ^ (bits(at.z) << 7);
        if (!rng) rng = 0x9E3779B9u;
        static const char* const kOften[] = {"Rare", "Now and then", "Often", "Always"};
        often = choice("how_often", kOften, 4, 1);
    }
    if (link != last_link) {  /* a file loaded, a stage entered: the stage took its actors; start over */
        last_link = link;
        beast = ctrl = NONE, slain_at = -1;
        settle = now + 2;
        if (warning) lv_windpower(0), warning = 0;
        until = 0;
    }
    const int settled = now >= settle && !play[WWHD_PLAY_ENABLE_NEXT_STAGE_OFFSET];

    const daPy_lk_c* player = (const daPy_lk_c*)link;
    const s32 proc = player->mCurProc;  /* riding the boat: SHIP_READY..SHIP_RESTART, not SHIP_GET_OFF */
    const int riding = proc >= 0x86 && proc <= 0x91 && proc != 0x90, swimming = (player->mModeFlg & 0x40000) != 0;
    const int at_sea = stage[0] == 's' && stage[1] == 'e' && stage[2] == 'a' && !stage[3];
    const int water = riding || swimming;
    const f32 time = *(volatile f32*)(save_info() + WWHD_OFFSET_save_info + 0x24);
    const double hour = time / 15;  /* degrees, 15 an hour from midnight */
    const int night = hour >= 19 || hour < 5;
    direct(play, link, night, water, at_sea, settled);

    /* Always: far out at sea, without signs; it goes when Link comes near land */
    const cXyz at = pos_of(link);
    if (always()) {
        const double dx = at.x - round_(at.x / 1e5) * 1e5, dz = at.z - round_(at.z / 1e5) * 1e5;
        const int out = at_sea && water && dx * dx + dz * dz > 6000.0 * 6000 && has_item(ITEM_BOOMERANG);
        if (!out && beast != NONE && slain_at < 0) remove_all();
        until = out ? now + LENGTH : 0;
    }

    /* the fog rolls in with the signs and lifts after, over 30 s: the world fades from 100 to 2500 around the camera,
     * blended with the game's own fog by the ratio; once more at 0 to hand the game its fog back */
    const int spell = warning || (until > 0 && !always());
    const f32 to = spell ? 1.f : 0.f, ease = (f32)(dt > 0 && dt < 0.2 ? dt : 1.0 / 30) / 30;
    fog = to > fog ? (fog + ease < to ? fog + ease : to) : (fog - ease > to ? fog - ease : to);
    if (fog > 0 || fogged) {
        lv_fog(100, 2500, fog);
        fogged = fog > 0;
    }

    if (until <= 0 || !at_sea || !settled) return;
    u8* gy = beast != NONE ? lv_search(beast) : 0;
    if (slain && slain_at < 0) {  /* (an ice arrow's kill may already have taken it) */
        slain_at = now;
        wwhd_log("leviathan: it is slain");
        loot();
    }
    /* its controller first, the Leviathan once that exists (a process asked for is found a few steps later) */
    if (!gy && slain_at < 0 && now >= next_make) {
        if (ctrl == NONE || !lv_search(ctrl)) {
            ctrl = make(link, GYCTRL_PROC, kGyCtrlParam, 0, 0, NONE);
            next_make = now + 2;
        } else {
            beast = make(link, GYORG_PROC, NONE, 2500, -40, ctrl);
            next_make = now + 10, health = HEALTH;  /* (if it is lost, another comes) */
            if (beast != NONE) wwhd_log("leviathan: it surfaces");
        }
        return;
    }
    if (slain_at >= 0) {  /* the game's own death plays at its size for 4 s, then it goes and the spell ends */
        if (now - slain_at > 4 || !gy) {
            finish("leviathan: ends, it is slain");
            if (always()) next_make = now + 600;  /* (not again at once) */
        }
        if (gy) beast_at = gy, ((fopAc_ac_c*)gy)->scale = (cXyz){SIZE, SIZE, SIZE};
        return;
    }
    if (!gy) return;
    beast_at = gy;
    /* 8x: its model takes the actor's scale each execute; its body collider's radius is a plain store each execute, so
     * it is multiplied after it (once: a step without its execute keeps ours); its spheres are sized in sph_set_r */
    ((fopAc_ac_c*)gy)->scale = (cXyz){SIZE, SIZE, SIZE};
    static f32 body_r;
    if (flt(gy, GY_BODY_R) != body_r) body_r = flt(gy, GY_BODY_R) * SIZE, set_flt(gy, GY_BODY_R, body_r);
    /* it circles the boat while its controller's clear-line flag for it is set (our quiet controller never sets it),
     * and every 10 s it breaches to ram */
    u8* c = (u8*)word(gy, GY_CTRL);
    const u32 child = word(gy, GY_CHILD);
    if (in_mem2((u32)c) && child < 8) c[CTRL_CLEAR + child] = 1;
    if (word(gy, GY_MODE) == 1 && now >= rammed + 10) lv_ram(gy), rammed = now;
    /* a sailing boat would leave it behind: it is kept within 2500 of Link */
    const cXyz g = pos_of(gy);
    const double far = dist2d(g, at);
    if (far > 2500) {
        fopAc_ac_c* actor = (fopAc_ac_c*)gy;
        actor->current.pos.x = (f32)(at.x + (g.x - at.x) * 2500 / far);
        actor->current.pos.z = (f32)(at.z + (g.z - at.z) * 2500 / far);
    }
}

/* ---- its health ---- */
/* daGy_c::checkTgHit: a Gyorg has 1 health; an arrow, the hookshot or the Boomerang take 2, a bomb sets it to 0, ice and
 * light arrows freeze it away, the sword does nothing. Before the test the Leviathan gets 100; after it, what was taken
 * is counted into its own health (once per 0.3 s: a bomb's burst lasts a few steps) and, while some is left, what the
 * game's death (or freezing) set is put back and it flinches as a Gyorg that survives a hit does. Its last hit is the
 * game's own death. */
#define KEPT 10
static const u32 kKept[KEPT] = {0x2E0, 0x340, 0x370, 0x39C, GY_MODE, GY_LAST_MODE, 0x600, 0x608, 0x60C, 0xA3C};
static u32 kept[KEPT];
static u8 kept_e31, kept_reg;
static int checking;
static u8* event_reg(void) { return save_info() + SAVE_EVENT + 0x7E; }  /* event register 0x7EFF: ice arrows count it */
WWHD_HOOK(WWHD_ADDR_daGy_c__checkTgHit_02168170, leviathan_hit_check, (u8* self)) {
    checking = self && self == beast_at && !slain;
    if (!checking) return;
    for (int i = 0; i < KEPT; i++) kept[i] = word(self, kKept[i]);
    kept_e31 = self[0xE31], kept_reg = *event_reg();
    self[GY_HEALTH] = 100;
}
WWHD_HOOK_RETURN(WWHD_ADDR_daGy_c__checkTgHit_02168170, leviathan_hit_checked, (u8* self)) {
    if (!checking) return;
    checking = 0;
    const s8 hp = (s8)self[GY_HEALTH];
    const u8 frozen = self[GY_FROZEN];
    int damage = hp < 100 ? (hp <= 0 ? 4 : 100 - hp) : 0;
    if (frozen == 9 || frozen == 0xB) damage = 4;
    if (!damage) return;
    if (now - last_hit >= 0.3) {
        last_hit = now;
        health -= damage;
        wwhd_log_int("leviathan: hit, health left", health);
    }
    if (health <= 0) {
        slain = 1;  /* the game's death (or freezing away) goes on */
        return;
    }
    const u32 mode = word(self, GY_MODE);
    self[GY_HEALTH] = 100, self[GY_FROZEN] = 0;
    if (mode == 8 || mode == 9) {
        for (int i = 0; i < KEPT; i++) set_word(self, kKept[i], kept[i]);
        self[0xE31] = kept_e31, *event_reg() = kept_reg;
        set_word(self, GY_LAST_MODE, word(self, GY_MODE));
        lv_flinch(self);
    }
}
/* its hit spheres (cM3dGSph::SetR, radius at +0x0C) as big as it is */
WWHD_HOOK_RETURN(WWHD_ADDR_sph_set_r_02018C8C, leviathan_sphere, (u8* sphere, f32 r)) {
    if (beast_at && sphere >= beast_at && sphere < beast_at + 0x1000) set_flt(sphere, 0x0C, r * SIZE);
}
