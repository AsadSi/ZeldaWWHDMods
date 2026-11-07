# Leviathan

Rarely, at night far out on the Great Sea, the wind dies away and fog rolls in. Thirty seconds later a Gyorg eight
times the size of any other surfaces behind the boat and hunts Link for three minutes, keeping up however fast he
sails: it circles the boat wide, dives to stalk him unseen, and now and then breaches and charges the boat.

- **Escape it** by making for an island, or hold out until it loses interest and the fog lifts.
- **Kill it** for 200 rupees and a blue potion in an empty bottle. It takes a lot of hitting: arrows, the hookshot
  and the Boomerang hurt it a little, bombs and light arrows more. The sword does nothing, as with any Gyorg.
- It only comes once Link has the sail, the Wind Waker and the Boomerang (the game makes no Gyorg before that),
  never in a file's first four minutes, and not again for twelve minutes after one ends.
- When it ends, the Leviathan goes with the fog. Nothing stays behind.

The Leviathan is the game's own Gyorg. Nothing from the game is included in this package.

## Options

| Option | Choices |
| --- | --- |
| How often | Rare, Now and then (default), Often, or Always (far out at sea day and night, without the signs) |

## How it works

Once per logic step, after the actors run, the mod rolls once a minute while Link sails or swims far from any island
at night. During the signs it lowers the wind (`dKyw_custom_windpower`) and blends in fog (`dKy_fog_startendz_set`);
then it makes a quiet Gyorg controller and one Gyorg as its child with `fopAcM_create` in Link's room and process
layer, 2500 units behind him, and keeps it at 8x (its scale, its body collider's radius, and its hit spheres). It
switches between three of the Gyorg's own modes at random: circling (`daGy_c::modeCircleInit`) held 3500-4500 units
from Link, diving (`modeDiveInit`) held 5000-6500 away, and charging the boat (`modeAttackInit`).

Its health (48) is counted around the game's own hit test: before `daGy_c::checkTgHit` it gets the game's health 100,
after it what the hit took is counted, and while some is left what the game's death or freezing set is put back and it
flinches as a Gyorg that survives a hit does. The last hit is the game's own death.

Hooks: `fpcEx_Handler` (return), `daGy_c::checkTgHit` (entry and return), `sph_set_r` (return: the Leviathan's own
hit spheres only). It replaces no function, so it can be on together with Haunted sea.

## Status

Experimental. Ported from the built-in Leviathan of
[wwhd-recomp-mods](https://github.com/AsadSi/wwhd-recomp-mods). The port to SDK v2 is new: see the pull request for
what has been tested.

Licence: MPL-2.0 (see LICENSE).
