# Graphics Concepts Used

This file lists the computer graphics techniques in the project, where each one appears
in the code, and how to show it during a presentation. A ready-made demo script is at
the end.

## Summary

| Concept | Main location |
|---|---|
| Geometric primitives | `Graphics.cpp` (all shapes), used by every object |
| Translation | Character and guard movement, camera (`Game::renderWorld`) |
| Rotation | Guard facing and vision cone, limb swing, gate panels, fans |
| Scaling | Mirrored side pose, UI button pulse, star pop, portraits |
| Matrix transformations | `gfx::pushTransform` = push / translate / rotate / scale |
| Orthographic projection | `gfx::beginFrame` (`glOrtho`), letterboxed viewport |
| Alpha blending | Vision cones, shadows, overlays, glows |
| Geometric collision | `Collision.cpp` circle–rectangle / circle–circle |
| Line intersection | `collision::raySegment`, `segmentsIntersect`, `lineOfSight` |
| Animation | Walk cycles, search keyframes, sway, flag wave, confetti |

## Phase 1: Graphics foundation

| Concept | Where | How to demonstrate |
|---|---|---|
| **Orthographic projection** | `gfx::beginFrame()` in `Graphics.cpp` calls `glOrtho(0,1000,0,600,-1,1)` | Explain that the game works in 1000×600 logical units, not pixels |
| **Viewport / letterboxing** | `gfx::setupProjection()` picks the largest 5:3 viewport that fits the window | Resize the window. The campus scales and never stretches, and dark bars fill the extra space |
| **Geometric primitives** | `Graphics.cpp`: `GL_QUADS` (rect), `GL_TRIANGLE_FAN` (circle, ellipse, rounded rect, sector), `GL_LINES` (grass tufts, paving joints), `GL_LINE_LOOP` (outlines), `GL_LINE_STRIP` (arc), `GL_QUAD_STRIP` (ring, flag), `GL_POLYGON`, `GL_POINTS` (wild flowers, asphalt grain) | Press **G** and point out that every object, even the trees, is made of circles and rectangles |
| **Procedural circle generation** | `gfx::ellipse()` places vertices at `(cx + cos θ·rx, cy + sin θ·ry)`. The segment count grows with on-screen size | Show the formula. Trees use 8 overlapping circles drawn in three passes: outline, fill, highlight |
| **Translation + rotation + scaling** | `gfx::pushTransform()` = `glPushMatrix` + `glTranslatef` + `glRotatef` + `glScalef` | Benches: one horizontal model is rotated 90° to make a vertical bench (`drawBench`) |
| **Hierarchical / local coordinates** | `drawGatePanel()` is drawn around its hinge at (0,0), then rotated into place. AC fans are drawn at the origin, then rotated | Gate panels swing open by changing only the rotation angle |
| **Animation by transformation** | Tree canopies sway (`glRotatef`) and breathe (`glScalef`). AC fans spin. The goal ring pulses (`glScalef`) | Watch the roof fans and the gold ring at the academic building door |
| **Per-vertex animation** | `drawFlagCloth()`: a `GL_QUAD_STRIP` whose vertex y-coordinates follow a sine wave | The Bangladesh flag waves, and the shading varies along the cloth |
| **Alpha blending** | `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` in `initRenderState()` | Shadows, cloud shadows drifting over the map, lamp glows |
| **Gradients (colour interpolation)** | `gradientRectV()` and `softEllipse()` use different colours per vertex, and OpenGL interpolates across the face | Window glass, the soft circular tree shadows and the lamp halos |
| **Painter's algorithm / layering** | `Game::render()`: ground, props, canopies, UI. Buildings are sorted back to front | Tree tops and the gate arch are drawn above everything else. Characters will walk *under* them |
| **Consistent lighting** | All shadows are offset to the bottom-right, as if light comes from the top-left | Point out that every shadow falls the same way |
| **Anti-aliasing** | `GLUT_MULTISAMPLE` + `GL_MULTISAMPLE`, `GL_LINE_SMOOTH` | Smooth circle edges |
| **Text** | Bitmap fonts (`glutBitmapString`) for labels. Stroke fonts (`glutStrokeCharacter`) scale with `glScalef` | Building name banners |
| **Fixed-timestep loop** | `main.cpp` `tick()`: an accumulator runs updates at 120 Hz | Animation speed is the same on fast and slow PCs |
| **Data vs. rendering** | `Level.cpp` holds only positions. `Map.cpp` turns them into geometry | Show a tree added with one line: `{{x, y}, radius, TreeKind::Rain}` |

## Phase 2: Student character and movement

| Concept | Where | How to demonstrate |
|---|---|---|
| **Hierarchical modelling** | `Player.cpp` `drawLeg()` / `drawArm()`: each limb is drawn in its own local space, pivoting at the hip or shoulder, inside the body's transform | Walk sideways: legs and arms swing around their joints |
| **Rotation animation** | Side view: leg angle = `sin(phase)·28°`, arm angle = `sin(phase)·32°` in the opposite direction (`glRotatef`) | Hold A or D and watch the stride |
| **Translation animation** | Body bob `abs(sin(phase))·1.3`. Feet lift in the front and back views | Hold W or S |
| **Scaling as reflection** | The right-facing pose is the left-facing model drawn with `glScalef(-1, 1)` (`Player::draw`) | Point out that only three poses are modelled, but there are four directions |
| **Separate collision shape** | `Player::RADIUS = 8`, a circle at the feet. The drawing is about 18×36 units | Press G: a green circle at the feet, while the body overlaps walls *visually* |
| **Circle–rectangle collision** | `collision::pushOutOfRect()`: find the closest point on the rectangle and push out along the normal | Walk into a building corner: the student slides around it |
| **Circle–circle collision** | `collision::pushOutOfCircle()` for tree trunks, lamp posts and tables | Walk into a lamp post |
| **Sub-stepped resolution** | `collision::moveCircle()` splits large moves so the student never tunnels through thin walls | — |
| **Vector normalization** | `Player::update()`: the input vector is normalized, so the diagonal speed is 115 units/s, not 163 | `./test.sh` prints both speeds |
| **Depth sorting (painter's algorithm by y)** | `Game::render()` collects `DepthItem`s (buildings, trunks, benches, lamps, the student), sorts them by base y and draws back to front | Walk behind the Shaheed Minar or a lamp post: the student is hidden behind it. Walk in front: the student is drawn over it |
| **Smooth, time-based motion** | Velocity eases toward the target (`vel += (target − vel)·min(1, dt·18)`) inside the 120 Hz fixed timestep | Movement is the same at any frame rate |

## Phase 3: Guard vision

| Concept | Where | How to demonstrate |
|---|---|---|
| **Ray casting / visibility polygon** | `Guard::drawVisionCone()` casts 72 rays across the field of view. `collision::raycast()` shortens each ray at the first wall. The ray end points form the rim of a `GL_TRIANGLE_FAN` | Watch the cone sweep past the Shaheed Minar or a building: the cone is cut off exactly at the wall |
| **Line intersection** | `collision::raySegment()` solves `o + d·t = a + e·u` with 2D cross products. `collision::segmentsIntersect()` uses the orientation test | `./test.sh` checks the ray stops at exactly 200.00 units |
| **Line of sight** | `collision::lineOfSight()` tests the guard-to-student segment against every edge of every sight blocker | Hide behind the stone sign or a building inside the cone: the meter drains instead of filling |
| **Angle test (dot product / atan2)** | `Guard::canSee()`: the angle to the student must be within ±fov/2 of the facing angle (with wrap-around handled by `angleDiff`) | Stand just outside the cone edge |
| **Same rules for drawing and logic** | The drawn cone and the detection use the same blockers, range and angle | You are only ever caught inside the visible cone |
| **Alpha blending + colour interpolation** | The cone fan fades from the guard to the rim (per-vertex alpha). Its colour goes yellow-green → orange → red with the detection level (`alertColor` uses linear `mix`) | Let the guard see you for a moment, then hide |
| **Rotation over time** | Facing angle = `base + sin(2πt/period)·sweep`. The pose (front/back/side) is chosen from that angle (`directionFromAngle`) | The guard turns his body as the cone sweeps |
| **Scaling** | The guard uses the same model as the student, scaled ×1.08 with a small breathing scale on y. The "?"/"!" bubble pops with an oscillating `glScalef` | Get spotted |
| **Shared hierarchical model** | `Character.cpp` draws both characters. A `CharacterStyle` switches the backpack for a cap, badge, belt, radio and moustache | Compare the student and the guard |

## Phase 4: Scrolling campus and guard AI

| Concept | Where | How to demonstrate |
|---|---|---|
| **Camera as a translation** | `Game::render()` wraps the whole world in `glTranslatef(-camera.x, -camera.y)`. The UI is drawn after `glPopMatrix` in dedicated bands outside the shorter world viewport | The 2000×1200 campus scrolls while the HUD stays put |
| **World vs. view coordinates** | `WORLD_W×WORLD_H` (2000×1200) is the campus. `VIEW_W×VIEW_H` (1000×600) is what `glOrtho` shows | Walk to a corner: the camera is clamped and never shows outside the campus |
| **Smooth camera follow** | `Game::updateCamera()`: `cam += (target − cam)·min(1, dt·6)` (exponential smoothing) | Stop suddenly: the camera eases in |
| **Waypoint movement (translation)** | `Guard::update()` Patrolling: move toward the next waypoint at `speed·dt` | Press G to see routes (blue lines, orange = search points) |
| **Smooth rotation** | `Guard::turnTowards()` limits the turn to N°/s along the shortest arc | Guards turn at corners instead of snapping |
| **Keyframe animation** | `searchProfile()`: look left → right → centre → pause, with smoothstep easing between keyframes | Watch a guard stop at the end of his route |
| **Finite state machine** | PATROLLING → SEARCHING → PATROLLING. Any state → ALERT on a brief sighting. ALERT → PATROLLING after 2.5 s unseen. Full meter → CAUGHT | Get seen briefly, then hide: "?" appears, the guard stops and stares, then resumes |

## Phase 6: UI screens and sprint

| Concept | Where | How to demonstrate |
|---|---|---|
| **Screen-space vs. world-space** | `Game::render()` draws the world inside the camera transform, then the HUD and menus after `glPopMatrix` in 1000×600 view units (`UI.cpp`) | Menus stay fixed while the campus pans behind them |
| **Scalable stroke text** | The title "ID Card Kothay?" and the "?" on the ID card are `glutStrokeCharacter` geometry, so they scale and rotate with the matrix | The ID card tilts and the "?" rotates with it |
| **Scaling animation** | The selected button pulses (`glScalef`). Level-complete stars pop in. The victory student squashes and stretches while jumping | Move through any menu |
| **Model reuse at a different scale** | The game-over guard portrait and the victory student are the in-game character models scaled ×4 and ×3 | Get caught |
| **Procedural particles** | Victory confetti: 90 quads whose position, fall speed and spin come from `hash01(i)` and time, so no particle data is stored | Finish Level 4 |
| **Translucent overlays** | Full-screen alpha-blended rectangles dim the scene behind menus | Pause |
| **Noise visualisation** | Expanding, fading ellipse rings at the student's feet while sprinting | Hold Shift |

## Phase 7: Timer and score

| Concept | Where | How to demonstrate |
|---|---|---|
| **Rotation driven by time** | The HUD clock hand is rotated by `−levelTime·6°` (one turn per minute) | Watch the clock icon next to the timer |
| **Fixed-timestep timing** | The level time accumulates the 120 Hz simulation steps, so it pauses exactly when the simulation does | Pause: the timer stops |

## Presentation demo script (about 5 minutes)

1. **Projection.** Start the game and resize the window. The 1000×600 view keeps its aspect ratio
   because of the letterboxed `glOrtho`.
2. **Primitives.** On the main menu, point out that the campus, characters, ID card and title
   are all geometry. The title and the "?" are stroke-font line segments.
3. **Transformations.** Start Level 1 and walk around.
   - Translation: the student and the camera move.
   - Rotation: legs and arms swing.
   - Scaling: the right-facing pose is the left pose drawn with `glScalef(-1, 1)`.
4. **Depth sorting.** Walk behind the Shaheed Minar or a lamp post (hidden), then in front (visible).
5. **Collision.** Press **G** to show the red collision boxes and the green feet circle. Walk into a
   building corner and slide along it.
6. **Vision cone and line intersection.** Start `--level 2`. Watch the plaza guard's cone get cut
   off by the Minar. Hide behind it while the meter drains, then step out while it fills
   (yellow-green → orange → red).
7. **AI state machine.** With **G** on, show a guard's route, his search stop (look left/right),
   and the alert "?" when he briefly sees you.
8. **Hearing.** Sprint (Shift) near a guard behind a wall. He turns towards the noise rings.
9. **Alpha blending.** Pause: the dimmed overlay. Get caught: the game-over screen uses the guard
   model scaled ×4.
10. **Finish.** Complete a level to show the score breakdown and popping stars, then run
    `--level 4` to the end for the confetti victory screen.
