# Architecture

## Data sources

The plugin consumes:

- Decoded `RunCommand` input, one user command per player per tick, with the
  mouse counts it carries and the pawn's predicted recoil punch, scope state
  and burst counter read as it arrives.
- `weapon_fire`, `bullet_impact`, `player_hurt`, `player_death`, and
  `player_spawn` events. The pawn names the command that fired, so a fire event
  binds to its exact command.
- Sight lines traced through the framework's `World.Trace` service: each tick,
  from every checked player to the enemy nearest their crosshair, and on a hit
  from the shooter's teammates to the victim.
- Relevant game-setting changes.
- Client convar responses and userinfo values.

## Source layout

The tree splits on the one boundary the build can check. `Detect/` touches no
engine type and compiles into `anticheat-tests` against header-only VoltMod;
`Engine/` is every contact point with the game. CMake globs `Detect/*.cpp` into
the test target, so the link step is what enforces the split: a file there that
reaches for `Log` or the SDK fails to link, and belongs in `Engine/`.

```text
plugins/anticheat/
  src/
    App.*          The object graph and the wiring between it
    Detect/        SDK-free: samples, geometry, view lag, weapon classes, findings,
                   the shot history and the suspicion score
      Rules/       The eleven detection rules, each named for what it detects
    Engine/        Every VoltMod contact point: the detection feed, sight lines,
                   command sampling, the cvar and name polls, the event scan,
                   status, commands, the usercmd dump and the cheat simulator
    Response/      Decision policy, actions, and Discord reporting
  configs/         Per-server settings and shared detection data
  docs/            These guides
  tests/           SDK-free rule, score and policy tests
```

Adding a detector touches three places, two of them guarded at compile time:
the `DetectionKind` enum and its catalog in `Detect/Finding.hpp`, the toggle
table in `Config.hpp`, and a member on `Detectors`. The call sites in
`Engine/DetectionFeed.cpp` stay explicit and unguarded, because the handlers
take different arguments per rule and a uniform signature would cost more in
clarity than the check is worth.

## How the pieces fit

`Detectors` owns every rule by value and answers the four questions an adapter
asks: detections on, rule on, slot eligible, report. Adapters hold a
`Detectors&`, so no header has to forward-declare the other side.

Rules live in `Anticheat::Rules`, which is what lets each be named for the cheat
it detects - `Rules::Aimbot`, `Rules::Wallhack` - without colliding with the
member on `Detectors` that holds it.

`Engine/DetectionFeed` is the single subscriber that drives every aim rule. The
three client-integrity checks poll on their own schedules instead, because they
ask the client something rather than watching it.

## Shot correlation

`Detect/ShotHistory` joins a command to the shot and the events it produced. A
`weapon_fire` event binds to the command the pawn names as its last firing
command; without that number, only when exactly one candidate is in the window,
and it discards ambiguous candidates.

Every shot is finalized once, two ticks after it fired, and each shot-reading
rule judges it then. It retains 128 ticks of position history, and leaves
missing input-history indices absent rather than clamping them to another
command's angles.

## Testing

`voltmod_add_tests` globs `src/Detect/*.cpp` into `anticheat-tests`, which links
`doctest` and header-only VoltMod and nothing else. Rules are constructed
directly by concrete type - no mocks, no interfaces - with a per-rule harness
struct that owns a `Suspicion` and reads values back in the rule's own units.

Rules report through `Suspicion::ReportTo` rather than returning a finding, so a
harness wires `Anticheat::Test::Findings` from `tests/Harness.hpp` into the score
and reads what came out. That header also holds the shared slots, frame builder
and constants every scenario uses.

Each harness has its own name (`WallhackHarness`, `AimlockHarness`), because two
same-named structs at namespace scope in one binary is an ODR violation the
linker resolves silently. An anonymous namespace would be the usual fix, but
`voltmod modgraph` rejects those, so distinct names it is.

Neither `App` nor the engine adapters are covered: the test target links no SDK,
so slot reuse, snapshot transfer, response execution and polling are verified by
hand on a live server.

```bash
uv run poe test                # build, then CTest
uv run poe test -R Suspicion   # one file
```
