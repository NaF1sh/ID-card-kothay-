#include "Audio.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

bool Audio::init() {
    if (!recording_.empty()) return true;
    // Resolve against the EXE, so shortcuts and other working directories work.
    std::vector<wchar_t> buffer(32768);
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length || length >= buffer.size()) return false;
    auto file = std::filesystem::path(std::wstring(buffer.data(), length)).parent_path()
                / L"assets" / L"audio" / L"guard_caught.wav";
    const char* cueFiles[] = {"pickup.wav", "complete.wav", "distraction.wav", "pet.wav", "alert.wav"};
    for (size_t i = 0; i < cues_.size(); ++i) {
        std::ifstream cue(file.parent_path() / cueFiles[i], std::ios::binary);
        cues_[i].assign(std::istreambuf_iterator<char>(cue), std::istreambuf_iterator<char>());
        if (cues_[i].size() < 44 || cue.bad()) cues_[i].clear();
    }
    std::ifstream stream(file, std::ios::binary);
    recording_.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    if (recording_.size() < 44 || stream.bad()) {
        recording_.clear();
        std::fprintf(stderr, "Guard audio: could not load assets/audio/guard_caught.wav\n");
        return false;
    }
    return true;
}

bool Audio::playCaught() {
    if (recording_.empty()) return false;  // missing audio never interrupts gameplay
    stop();
    // The buffer stays alive until playback is stopped. No loop or blocking wait.
    playing_ = PlaySoundA(recording_.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT) != FALSE;
    if (!playing_) std::fprintf(stderr, "Guard audio: playback unavailable\n");
    return playing_;
}

void Audio::playCue(SoundCue cue) {
    const auto& sample = cues_[static_cast<size_t>(cue)];
    if (sample.empty()) return;
    playing_ = PlaySoundA(sample.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT) != FALSE;
}

void Audio::stop() {
    if (playing_) PlaySoundA(nullptr, nullptr, 0);
    playing_ = false;
}

Audio::~Audio() {
    stop();  // release the audio device before the recording buffer is destroyed
}
#else
bool Audio::init() { return false; }
bool Audio::playCaught() { return false; }
void Audio::playCue(SoundCue) {}
void Audio::stop() {}
Audio::~Audio() = default;
#endif
