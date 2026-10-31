# Ranked Search & Destroy implementation status

This commit introduces the first MW2 Recompiled-side code for the agreed Ranked rules: a deterministic rules/economy module under `runtime/`, plus an isolated test harness.

## Included

- MR15 regulation progression: 30 rounds, first to 16, side switch after round 15.
- MR3 overtime: six-round periods, side switch after three, first to four, repeat after a 3-3 period.
- Economy counters, capped money, CS:GO-style round/objective/kill rewards.
- Purchase prices and faction availability for the agreed MW2 roster.
- Frag plus one tactical grenade, armour/helmet and defender-only kit purchase rules.
- Round/plant/defuse timing constants.

The main CMake project already includes `runtime/*.cpp` with `CONFIGURE_DEPENDS`, so the new `runtime/ranked_rules.cpp` is included in subsequent configured builds without editing the root CMake file.

## Test

Run the isolated test harness:

```sh
cmake -S tests/ranked -B build-ranked
cmake --build build-ranked --config Release
ctest --test-dir build-ranked --output-on-failure
```

## Not yet playable

This is deliberately a rules layer only. It is not yet connected to the multiplayer engine's round, bomb, death, inventory, weapon-drop or buy-menu events. It does not add a Ranked menu entry, alter fastfiles, enforce 5v5 team sizes, disable perks/killstreaks in the running game, or provide matchmaking, a dedicated authoritative host, anti-cheat, ratings or leaderboards.

The MW2 runtime repository does not contain the retail game executables or extracted title-update-6 fastfiles. Engine hook addresses and menu/zone changes must be identified against a local build made from the user's own legally obtained game files; they must not be guessed.

## Safety and integration rule

Only call `MatchProgress::RecordRound` after the match host/server has validated the round result. The current peer relay is not an authoritative match server, so this module must not be treated as a ranked result validator.
