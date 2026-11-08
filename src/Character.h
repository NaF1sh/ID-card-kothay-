// Character.h - shared humanoid renderer for the student and the guards.
// One articulated model (legs/arms pivot at hip/shoulder) is configured by
// a CharacterStyle, so both characters share skeleton and animation code.
#pragma once

#include "Types.h"

struct CharacterStyle {
    Color skin, skinDark, hair, hairHi;
    Color shirt, shirtShade, pants, pantsFar, shoe, sole, outline;
    // student gear
    bool backpack = false;
    Color pack, packShade, zip;
    // security gear: cap, badge, belt, moustache, radio
    bool guardGear = false;
    Color cap, capDark, badge, belt;
};

// The two character looks used in the game.
const CharacterStyle& studentStyle();
const CharacterStyle& guardStyle();

// Draws the character with its feet at the local origin (y up).
// walkPhase drives the limb swing, moveBlend (0..1) its amplitude.
void drawCharacter(const CharacterStyle& s, Direction facing, float walkPhase, float moveBlend);

// Soft contact shadow under the feet (not affected by walk bob).
void drawCharacterShadow(Vec2 feet, float scale);

// Maps a continuous facing angle (degrees, 0 = +x, counter-clockwise)
// to one of the four drawn poses.
Direction directionFromAngle(float deg);
