# Call of the Sky — SDK v2 example

An SDK v2 PowerPC guest example of the optional Dragon Roost quest and Valoo ride.
The original built-in prototype remains in `../legacy/dragon/`.

Install through Mods → Browse, complete the guest build, enable code mods and
restart. The pinned SDK in `../sdk.json` is required; the release number alone
does not establish compatibility. USA and European address translation is
performed by the port. Panels use the shared Metal/Vulkan HUD service.

The quest "Valoo's Gratitude" is in the port's story notebook (declared in `manifest.json`). Once
Link has the Deku Leaf and has helped Valoo, Medli's letter arrives by Rito mail at any postbox;
reading it starts the quest. Meet Medli at Dragon Roost's high entrance and follow the chime and song
instructions; the notebook shows the current step and, with "Show on chart", where to go. B cancels
dialogue or a ride request. Progress is kept in the save file (the port's story record): it follows
the Quest Log, its copies, backups and save states. Progress that earlier versions kept in this
mod's data folder (`dragon-quest-slotN.txt`) is moved into the save once, the first time that Quest
Log is played.

The synthesized song melody is omitted. The game sound calls used for chimes
do not provide an established interface for this custom melody. See
[the proposed generic audio service](../docs/audio-stream-service.md).

This mod is marked experimental: the port lists it only with **Show experimental mods** on,
warns before enabling it and backs up your saves first (restore them in Mods → Save backups).
Do not rely on saves made while it is active.

This example remains a prototype. USA/EU Metal/Vulkan timing checks, cancellation,
boat/leaf recovery, occupied-neighbour save-slot preservation and same-run state
restoration passed within the documented test fixtures. Quest and ride checks
include instrumented travel fixtures; a complete ordinary quest playthrough is
not established. The opt-in `tests/run_runtime_e2e.py` harness records private
evidence and distinguishes normal routes from instrumented fixtures. No game
assets or saves are included.

Known limitations: not checked against every story scene that uses Medli or Valoo.
