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

static void faceHorizontally(float ax)
{
    if (ax < 0)
        player.facing = DIR_LEFT;
    else
        player.facing = DIR_RIGHT;
}

static void faceVertically(float ay)
{
    if (ay < 0)
        player.facing = DIR_DOWN;
    else
        player.facing = DIR_UP;
}

static void updateFacing(float ax, float ay)
{
    if (ax == 0 && ay == 0)
        return;

    bool facingMoveX = (ax < 0 && player.facing == DIR_LEFT) || (ax > 0 && player.facing == DIR_RIGHT);
    bool facingMoveY = (ay < 0 && player.facing == DIR_DOWN) || (ay > 0 && player.facing == DIR_UP);
    bool alreadyFacingMove = facingMoveX || facingMoveY;

    if (ay == 0 || (ax != 0 && !alreadyFacingMove))
        faceHorizontally(ax);
    else if (ax == 0)
        faceVertically(ay);
}

static void updateStamina(float dt, bool sprint, bool wantsMove)
{
    if (player.exhausted && player.stamina > 0.3f)
        player.exhausted = false;

    player.sprinting = sprint && wantsMove && !player.exhausted && player.stamina > 0;
    if (player.sprinting)
    {
        player.stamina -= STAMINA_DRAIN * dt;
        if (player.stamina <= 0)
        {
            player.stamina = 0;
            player.exhausted = true;
        }
    }
    else
    {
        player.stamina = minf(1, player.stamina + STAMINA_REGEN * dt);
    }
}

static void updateVelocity(float dt, float ax, float ay, bool wantsMove)
{
    Vec2 dir = vnormal(makeVec(ax, ay));
    float speedMultiplier = 1.0f;
    if (player.sprinting)
        speedMultiplier = SPRINT_MULT;
    float speed = PLAYER_SPEED * speedMultiplier;
    Vec2 targetVel = vscale(dir, speed);

    float smoothing = minf(1, dt * 32);
    Vec2 change = vscale(vsub(targetVel, player.vel), smoothing);
    player.vel = vadd(player.vel, change);
    if (!wantsMove)
        player.vel = makeVec(0, 0);
}

static void updateWalkAnimation(float dt, float actualSpeed)
{
    float speedRatio = actualSpeed / PLAYER_SPEED;
    float targetBlend = 0;
    if (actualSpeed > 8)
        targetBlend = minf(1, speedRatio);

    player.moveBlend += (targetBlend - player.moveBlend) * minf(1, dt * 12);
    player.walkPhase += dt * 12 * maxf(player.moveBlend, speedRatio);
    if (player.walkPhase > 2 * PI)
        player.walkPhase -= 2 * PI;
}

void updatePlayer(float dt, float ax, float ay, bool sprint)
{
    bool wantsMove = (ax != 0 || ay != 0);
    updateFacing(ax, ay);
    updateStamina(dt, sprint, wantsMove);
    updateVelocity(dt, ax, ay, wantsMove);

    Vec2 newPos = moveCircle(player.pos, vscale(player.vel, dt), PLAYER_RADIUS);
    float actualSpeed = vlength(vsub(newPos, player.pos)) / dt;
    player.pos = newPos;

    updateWalkAnimation(dt, actualSpeed);
}

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
