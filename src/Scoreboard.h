#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct ScoreEntry {
    std::string name;
    int score = 0;
    float seconds = 0;
};

// Local completed runs, ordered by score, then fastest total time.
class Scoreboard {
public:
    explicit Scoreboard(std::filesystem::path path = defaultPath()) : path_(std::move(path)) {}
    static std::filesystem::path defaultPath();
    static std::string cleanName(const std::string& name);
    void load();
    bool record(ScoreEntry entry);
    const std::vector<ScoreEntry>& entries() const { return entries_; }
private:
    std::filesystem::path path_;
    std::vector<ScoreEntry> entries_;
};
