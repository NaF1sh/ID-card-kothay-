# Museum Detective Case

2D top-down stealth game made with OpenGL and GLUT (Computer Graphics lab project).
You are a detective: collect the evidence in each of the 4 museum levels and reach
the gold exit without being caught by the guards.

## Files

| File | What it does |
|---|---|
| `game.h` | Constants, structures, global variables, function prototypes |
| `main.cpp` | Global variables, `init()`, `display()`, keyboard, mouse, reshape, timer, `main()` |
| `drawing.cpp` | Vector math, colours, `DrawCircle` and other primitives, text |
| `collision.cpp` | Circle/rectangle collision, line of sight, A* path finding |
| `level.cpp` | The 4 levels: buildings, trees, guards, evidence, exit |
| `map.cpp` | Draws the scenery (grass, roads, lake, buildings, trees, gate...) |
| `characters.cpp` | Draws people, NPCs and cats; NPC dialogue lines |
| `player.cpp` | Player movement, sprint and stamina |
| `guard.cpp` | Guard patrol, vision cone, detection, chase and search |
| `gameplay.cpp` | Game flow, scoring, collecting, talking, distractions, depth-sorted world |
| `ui.cpp` | Menus, HUD, case file, dialogue box, result screens |
| `scoreboard.cpp` | Top 10 saved in `scoreboard.txt` |
| `sound.cpp` | Sound effects from `assets/audio` (Windows only) |

## Build

Windows (MSYS2 / Code::Blocks with freeglut):

    g++ -std=c++17 *.cpp -o MuseumDetective.exe -lfreeglut -lopengl32 -lglu32 -lwinmm

Linux:

    g++ -std=c++17 *.cpp -o MuseumDetective -lglut -lGL -lGLU

Run it from this folder so it finds `assets/audio`.

## Controls

| Key | Action |
|---|---|
| W A S D / Arrows | Move |
| Shift / Space | Sprint (fast but noisy) |
| F | Collect evidence / talk / pet a cat |
| Mouse click (or E) | Throw a distraction (2 per attempt) |
| J | Case file |
| P | Pause |
| R | Retry level (-100 points) |
| G | Debug view (collision boxes, guard routes) |
| Esc | Main menu |

## Scoring

1000 per level + time bonus (up to 600) + stealth bonus (500 if never spotted,
250 if never close to caught) - 100 per retry.
