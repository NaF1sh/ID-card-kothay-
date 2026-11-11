#include "game.h"

#define EFFECT_COUNT 5

const char *soundFiles[EFFECT_COUNT] = {
    "assets/audio/pickup.wav",
    "assets/audio/complete.wav",
    "assets/audio/distraction.wav",
    "assets/audio/pet.wav",
    "assets/audio/alert.wav"};

#define TRACK_MENU 0
#define TRACK_CALM 1
#define TRACK_CHASE 2
#define TRACKS 3
#define NO_TRACK -1

const char *musicFiles[TRACKS] = {
    "assets/audio/menu.mp3",
    "assets/audio/music.mp3",
    "assets/audio/chase.mp3"};

#define MUSIC_VOLUME 50

bool musicOn = true;
bool musicPaused = false;
bool chaseMusic = false;
bool menuMusic = true;

void updateMusic();

static bool isValidCue(int cue)
{
    return cue >= 0 && cue < EFFECT_COUNT;
}

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

const char *trackAlias[TRACKS] = {"menu", "calm", "chase"};

void playSound(int cue)
{
    if (!isValidCue(cue))
        return;
    PlaySoundA(soundFiles[cue], NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void stopSound()
{
    PlaySoundA(NULL, NULL, 0);
}

static void openPausedTrack(int track)
{
    const char *alias = trackAlias[track];
    char command[200];

    sprintf(command, "open \"%s\" type mpegvideo alias %s", musicFiles[track], alias);
    mciSendStringA(command, NULL, 0, NULL);

    sprintf(command, "setaudio %s volume to %d", alias, MUSIC_VOLUME * 10);
    mciSendStringA(command, NULL, 0, NULL);

    sprintf(command, "play %s repeat", alias);
    mciSendStringA(command, NULL, 0, NULL);

    sprintf(command, "pause %s", alias);
    mciSendStringA(command, NULL, 0, NULL);
}

void startMusic()
{
    for (int i = 0; i < TRACKS; i++)
        openPausedTrack(i);
    updateMusic();
}

void trackPlay(int track, bool play)
{
    char command[60];
    if (play)
        sprintf(command, "resume %s", trackAlias[track]);
    else
        sprintf(command, "pause %s", trackAlias[track]);
    mciSendStringA(command, NULL, 0, NULL);
}

void stopMusic()
{
    char command[60];
    for (int i = 0; i < TRACKS; i++)
    {
        sprintf(command, "close %s", trackAlias[i]);
        mciSendStringA(command, NULL, 0, NULL);
    }
}

#else
#include <unistd.h>
#include <signal.h>

pid_t effectPid = 0;
pid_t trackPid[TRACKS] = {0, 0, 0};

static void waitUntilGroupLeader(pid_t pid)
{
    for (int i = 0; i < 200; i++)
    {
        if (getpgid(pid) == pid)
            return;
        usleep(1000);
    }
}

pid_t runInBackground(const char *command)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        setsid();
        signal(SIGCHLD, SIG_DFL);
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(0);
    }
    if (pid <= 0)
        return 0;
    waitUntilGroupLeader(pid);
    return pid;
}

void playSound(int cue)
{
    if (!isValidCue(cue))
        return;
    char command[200];
    sprintf(command, "exec paplay --volume=65536 '%s' 2>/dev/null", soundFiles[cue]);
    effectPid = runInBackground(command);
}

void stopSound()
{
    if (effectPid > 0)
        kill(-effectPid, SIGTERM);
    effectPid = 0;
}

void stopMusic()
{
    for (int i = 0; i < TRACKS; i++)
    {
        if (trackPid[i] > 0)
        {
            kill(-trackPid[i], SIGCONT);
            kill(-trackPid[i], SIGTERM);
        }
        trackPid[i] = 0;
    }
}

void quitOnSignal(int sig)
{
    (void)sig;
    stopMusic();
    _exit(0);
}

pid_t loopTrack(const char *file)
{
    if (access(file, R_OK) != 0)
        return 0;

    int gamePid = (int)getpid();
    int paplayVolume = MUSIC_VOLUME * 65536 / 100;
    char command[400];
    sprintf(command,
            "game=%d; while kill -0 $game 2>/dev/null; do "
            "paplay --volume=%d --latency-msec=150 '%s' 2>/dev/null & p=$!; "
            "while kill -0 $p 2>/dev/null; do kill -0 $game 2>/dev/null || kill $p; sleep 1; done; "
            "wait $p || exit; done",
            gamePid, paplayVolume, file);
    return runInBackground(command);
}

void trackPlay(int track, bool play)
{
    if (trackPid[track] <= 0)
        return;
    if (play)
        kill(-trackPid[track], SIGCONT);
    else
        kill(-trackPid[track], SIGSTOP);
}

void startMusic()
{
    signal(SIGCHLD, SIG_IGN);
    for (int i = 0; i < TRACKS; i++)
    {
        trackPid[i] = loopTrack(musicFiles[i]);
        trackPlay(i, false);
    }
    updateMusic();
    atexit(stopMusic);
    signal(SIGINT, quitOnSignal);
    signal(SIGTERM, quitOnSignal);
    signal(SIGHUP, quitOnSignal);
}

#endif

static int wantedTrack()
{
    if (!musicOn || musicPaused)
        return NO_TRACK;
    if (menuMusic)
        return TRACK_MENU;
    if (chaseMusic)
        return TRACK_CHASE;
    return TRACK_CALM;
}

void updateMusic()
{
    int track = wantedTrack();
    for (int i = 0; i < TRACKS; i++)
        trackPlay(i, i == track);
}

void toggleMusic()
{
    musicOn = !musicOn;
    updateMusic();
}

void pauseMusic(bool paused)
{
    if (paused == musicPaused)
        return;
    musicPaused = paused;
    updateMusic();
}

void setChaseMusic(bool chase)
{
    if (chase == chaseMusic)
        return;
    chaseMusic = chase;
    updateMusic();
}

void setMenuMusic(bool menu)
{
    if (menu == menuMusic)
        return;
    menuMusic = menu;
    updateMusic();
}
