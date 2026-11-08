// ============================================================
// sound.cpp
// Short sound effects from assets/audio (Windows only, using
// PlaySound from winmm). On other systems the game is silent.
// ============================================================

#include "game.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

void playSound(int cue)
{
    const char *files[6] = {
        "assets/audio/pickup.wav",
        "assets/audio/complete.wav",
        "assets/audio/distraction.wav",
        "assets/audio/pet.wav",
        "assets/audio/alert.wav",
        "assets/audio/guard_caught.wav"};
    if (cue < 0 || cue > 5)
        return;
    // SND_ASYNC: play in the background, the game keeps running
    // SND_NODEFAULT: if the file is missing, stay silent
    PlaySoundA(files[cue], NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void stopSound()
{
    PlaySoundA(NULL, NULL, 0);
}

#else

void playSound(int cue)
{
    (void)cue;
}

void stopSound()
{
}

#endif
