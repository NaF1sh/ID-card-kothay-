# ID Card Kothay?

A campus stealth game written in **C++20, OpenGL and FreeGLUT**. You forgot your
student ID. Build a temporary-ID application across four campus stops and reach
class before the bell, while campus security tries to catch you.

Every visual is drawn in code with OpenGL primitives and GLUT fonts. No images,
textures, sprites or models are required.

## Play

Open the latest build, `ID Card Kothay.exe`, choose **Start Game**, and enter
your name. Source builds
using the commands below produce `IDCardKothay.exe`. Names support
1-18 English letters, numbers, spaces, hyphens and underscores.

- Read your application briefing. Each level and retry waits for your first move or
  successful interaction before starting the patrols and countdown.
- Follow the **cyan arrow** to a document stand. Later-stage guards begin well away
  from the arrival doorway, leaving room to get oriented and move out.
- Press **F** nearby to pick up its named document, with a sound and a short visual effect.
- Collect the required documents and reach the **gold entrance** for approval.
- Press **J** at any time to see your checklist and completed approval stamps.
- Progress through Library slip, Science verification, witness signature, and final ID
  submission. Each new stage starts near the previous destination, with a fresh timer.
- Finish before the countdown reaches zero. Pause and conversations stop the clock.
- Walk quietly, watch patrols, and use the short campus walls to break sight.
- **Aim with the mouse and left-click** to throw a distraction at the landing marker.
  Throws reach up to 190 world units and stop before obstacles. HUD and letterbox
  clicks do not throw. **E** also throws at the mouse aim; before any mouse input,
  E retains the facing-direction keyboard fallback. You get two distractions per attempt. Nearby guards investigate the noise;
  guards who can already see you keep chasing.
- Complete all four levels to save a named run to the **Scoreboard**.

| Key | Action |
|---|---|
| WASD / Arrow keys | Move |
| Shift / Space | Sprint; drains stamina and makes noise |
| Mouse / Left click | Aim / throw a clink (two per attempt) |
| E | Alternate throw key |
| F | Collect a document, talk to a character, or briefly pet a nearby cat |
| J | Open / close the application checklist (pauses gameplay) |
| F / Enter / Space / click during conversation | Reveal the speech, then close it |
| Esc during conversation | Leave conversation |
| P | Pause / resume |
| R | Restart the current level, with a 100-point penalty |
| Enter / Mouse click | Select a menu button |
| Esc | Main menu; quit from the main menu |
| G | Debug collision shapes and guard routes |

## Campus life

The scrolling 2000 x 1200 campus includes the Library, Science Block, Canteen,
Academic Building, Book Shop, Club Room, Student Hub, Arts Studio and Campus
Clinic. A lake with ripples, lilies and a duck sits beside the central lawn.
The football ground has goals, pitch markings, a moving ball and four players.
Twelve students walk campus routes and four cats wander around.

Face a named character within 65 world units to see **F: Talk**. Each character
has a sequence of short speeches, including jokes and useful gameplay hints. Each
interaction shows only one speech with a portrait. Reveal it, close it, then talk
again when you want the next speech:

| Character | Location | Personality |
|---|---|---|
| Rafi | Library entrance | Topper, glasses, backpack, always reading |
| Bappi | South side of the football ground | Football captain with a ball |
| Mintu Mama | Beside the canteen | Tea seller and campus gossip |
| Nila | East shore of the lake | Poet and duck enthusiast |
| Dr. Nap | Science Block entrance | Sleepy professor |
| Pixel | Near the Student Hub | Gamer with headphones |

Walking students and football players are ambient characters; named characters
have dialogue. Rafi, Dr. Nap, and Mintu Mama also explain the missing documents
for their stage. Changed task advice gets its own interaction and is not repeated
before every speech. Conversations freeze the simulation and countdown while you read.

Biscuit, Shadow, Mishti, and Professor Meow are small, four-legged campus cats.
They keep wandering when approached and occasionally rest. Face a cat within
38 units and press F for a brief crouch, head scratch, purr, and small heart.
Petting stays in the world: guards and the clock continue, and movement cancels
the player animation immediately. Cats resume wandering after a short pause.

Movement accelerates promptly and stops immediately on release. Pickup, approval,
distraction, petting, and initial sighting have quiet audio cues.

The HUD occupies dedicated top and bottom bands outside the campus viewport.
Passes, distractions, hints, and meters never cover the playable view.

## Security and challenge

Guards patrol, scan at waypoints, react to footsteps and distractions, chase
visible players, and investigate the last known location after losing sight.
They use collision-aware A* paths to navigate around buildings, walls and the lake.
Confirmed sightings (40% suspicion) are radioed to guards within 480 units. Radio reports share
a position, not vision through walls. Guards return to patrol after their search.

Vision cones are clipped to buildings and walls. Detection rises while visible,
faster at short range, and fades behind cover. Filling the meter means capture.
Guards outrun normal walking; sprinting is faster than pursuit but drains stamina
and makes noise. Use cover, patrol timing, and distractions to create openings.
A successful noise diversion by itself does not count as being spotted.

Level 1 keeps a safe first document and one patrol, with shorter vision and hearing
and more warning time. The guard patrols the library approach, so the second
document requires attention. Later stages restore the busier patrol patterns.

| Level | Destination | Guards | Documents | Time limit |
|---|---|---|---|---|
| 1. The Main Gate | Library | 1 | 2 | 90 seconds |
| 2. Find Another Route | Science Block | 3 | 2 | 85 seconds |
| 3. Patrol Timing | Canteen | 4 | 3 | 100 seconds |
| 4. ID Card Kothay? | Academic Building | 5 | 3 | 110 seconds |

## Scores

Each level awards 1000 points, up to 600 points for the fraction of time remaining,
and a stealth bonus: 500 if no guard became alert, 250 if nobody reached high
detection, otherwise zero. Each restart subtracts 100 points from that level,
with a minimum level score of zero. Stars: 1700+ earns three, 1300+ earns two.

The local scoreboard keeps the **10 best completed four-level runs**, sorted by
score and then fastest total time. Failed attempt time is included in total time.
Abandoned runs and command-line practice runs are not recorded. Repeated runs by
the same player are allowed.

Scores are stored in `scoreboard.txt` beside the executable, independent of the
working directory. Saves replace the old file only after writing the new file.
Missing or malformed records do not prevent play. If saving fails, the result
screen says so and retains scores for the current session.

## Build and test

Install MSYS2 UCRT64 GCC and FreeGLUT:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
./build.sh
./test.sh
```

For native PowerShell, with UCRT64 `g++` on PATH:

```powershell
g++ -std=c++20 -O2 -Wall -Wextra -DFREEGLUT_STATIC src/*.cpp -o IDCardKothay.exe -static -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm -mwindows
powershell -NoProfile -ExecutionPolicy Bypass -File ./test.ps1
```

Close the game before rebuilding. FreeGLUT and the runtime are linked statically.
Keep `assets/` beside the executable for the recorded guard voice and interaction sounds. Capture plays
`assets/audio/guard_caught.wav` once; restart and menu stop it. Missing audio never
interrupts gameplay.

Four windowless test programs cover movement, collision and reachability of every
exit/pass, vision and pursuit, audio transitions, obstacle navigation, names,
single-speech NPC interactions, safe arrival/retry states, application progression, cat interaction, countdowns, distraction limits, scoring and persistent
score files. Tutorial checks complete a west-side bypass at three patrol timings using a
distraction and a short sprint. Other checks cover warning/capture behavior, immediate
movement stopping, deliberate pickup, and keeping the stealth bonus after a diversion.

| Command-line option | Purpose |
|---|---|
| `--level N` | Start a practice level (1-4) |
| `--start x,y` | Place the player at a world position |
| `--screenshot out.bmp [seconds]` | Capture a frame and exit |
| `--hold keys` | Send initial key presses for deterministic previews |

## Source layout

- `Game.*`: state transitions, input, gameplay, camera, scoring and rendering.
- `Guard.*`, `Navigation.*`: detection, pursuit, investigation and obstacle navigation.
- `Player.*`, `Collision.*`: movement, stamina and collision handling.
- `Level.*`, `Map.*`: campus layout, level rules and environment rendering.
- `Campus.*`, `Character.*`: named NPCs, dialogue data, cats and character models.
- `Scoreboard.*`: validated local score storage and ranking.
- `UI.*`, `Graphics.*`: menus, HUD, dialogue and primitive drawing helpers.
- `Audio.*`, `main.cpp`: voice playback, application setup and fixed timestep loop.

`GRAPHICS_CONCEPTS.md` describes the graphics techniques used in the project.

