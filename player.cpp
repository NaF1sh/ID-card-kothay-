// ============================================================
// player.cpp
// The detective: smooth movement, sprint with stamina,
// sliding collision, and the walk animation.
// ============================================================

#include "game.h"

void resetPlayer(Vec2 spawn)
{
    player.pos = spawn;
    player.vel = makeVec(0, 0);
    player.facing = DIR_UP;
    player.walkPhase = 0;
    player.moveBlend = 0;
    player.stamina = 1;
    player.sprinting = false;
    player.exhausted = false;
}

// ax, ay: held direction keys (-1, 0 or 1)
void updatePlayer(float dt, float ax, float ay, bool sprint)
{
    // ---- facing direction ----
    if (ax != 0 || ay != 0)
    {
        bool horizontalOk = (ax < 0 && player.facing == DIR_LEFT) || (ax > 0 && player.facing == DIR_RIGHT);
        bool verticalOk = (ay < 0 && player.facing == DIR_DOWN) || (ay > 0 && player.facing == DIR_UP);
        if (ay == 0 || (ax != 0 && !horizontalOk && !verticalOk))
            player.facing = ax < 0 ? DIR_LEFT : DIR_RIGHT;
        else if (ax == 0)
            player.facing = ay < 0 ? DIR_DOWN : DIR_UP;
        // diagonal that matches the current facing: keep it (no flicker)
    }

    // ---- sprint and stamina ----
    bool wantsMove = (ax != 0 || ay != 0);
    if (player.exhausted && player.stamina > 0.3f)
        player.exhausted = false;
    player.sprinting = sprint && wantsMove && !player.exhausted && player.stamina > 0;
    if (player.sprinting)
    {
        player.stamina -= STAMINA_DRAIN * dt;
        if (player.stamina <= 0)
        {
            player.stamina = 0;
            player.exhausted = true; // must rest before sprinting again
        }
    }
    else
    {
        player.stamina = minf(1, player.stamina + STAMINA_REGEN * dt);
    }

    // ---- velocity ----
    // normalising keeps diagonal speed the same as straight speed
    Vec2 dir = vnormal(makeVec(ax, ay));
    float speed = PLAYER_SPEED * (player.sprinting ? SPRINT_MULT : 1.0f);
    Vec2 target = vscale(dir, speed);
    float k = minf(1, dt * 32);
    player.vel = vadd(player.vel, vscale(vsub(target, player.vel), k));
    if (!wantsMove)
        player.vel = makeVec(0, 0); // releasing the keys stops at once

    // ---- move with collision ----
    Vec2 newPos = moveCircle(player.pos, vscale(player.vel, dt), PLAYER_RADIUS);
    float actualSpeed = vlength(vsub(newPos, player.pos)) / dt;
    player.pos = newPos;

    // ---- animation follows the real speed (no walking into walls) ----
    float targetBlend = actualSpeed > 8 ? minf(1, actualSpeed / PLAYER_SPEED) : 0;
    player.moveBlend += (targetBlend - player.moveBlend) * minf(1, dt * 12);
    player.walkPhase += dt * 12 * maxf(player.moveBlend, actualSpeed / PLAYER_SPEED);
    if (player.walkPhase > 2 * PI)
        player.walkPhase -= 2 * PI;
}

// crouch (0..1) squashes the body while petting a cat
void drawPlayer(float crouch)
{
    float bob = fabsf(sinf(player.walkPhase)) * 1.3f * player.moveBlend;
    drawCharacterShadow(player.pos, 1.0f);

    glPushMatrix();
    glTranslatef(player.pos.x, player.pos.y + bob, 0);
    glScalef(1 + 0.08f * crouch, 1 - 0.35f * crouch, 1);
    useStyle(STYLE_DETECTIVE);
    drawCharacter(player.facing, player.walkPhase, player.moveBlend);
    glPopMatrix();
}
