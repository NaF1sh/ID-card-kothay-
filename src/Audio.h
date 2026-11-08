// Audio.h - the supplied guard voice, played asynchronously with Windows audio.
#pragma once

#include <vector>
#include <array>

enum class SoundCue { Pickup, Complete, Distraction, Pet, Alert };

class Audio {
public:
    Audio() = default;
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    bool init();  // preloads assets/audio/guard_caught.wav beside the executable
    bool playCaught();  // play once, starting from the beginning
    void playCue(SoundCue cue);
    void stop();

private:
    std::vector<char> recording_;
    std::array<std::vector<char>, 5> cues_;
    bool playing_ = false;
};
