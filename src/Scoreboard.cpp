#include "Scoreboard.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
void rankEntries(std::vector<ScoreEntry>& entries) {
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.score != b.score ? a.score > b.score : a.seconds < b.seconds;
    });
    if (entries.size() > 10) entries.resize(10);
}
}

std::filesystem::path Scoreboard::defaultPath() {
#ifdef _WIN32
    wchar_t buffer[32768];
    DWORD n = GetModuleFileNameW(nullptr, buffer, 32768);
    if (n && n < 32768) return std::filesystem::path(std::wstring(buffer, n)).parent_path() / "scoreboard.txt";
#endif
    return "scoreboard.txt";
}

std::string Scoreboard::cleanName(const std::string& name) {
    std::string clean;
    for (unsigned char c : name) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_') clean += c;
        if (clean.size() == 18) break;
    }
    auto first = clean.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    return clean.substr(first, clean.find_last_not_of(' ') - first + 1);
}

void Scoreboard::load() {
    entries_.clear();
    std::ifstream file(path_);
    std::string line;
    for (int i = 0; i < 1000 && std::getline(file, line); ++i) {
        std::istringstream row(line);
        ScoreEntry e;
        if (!(row >> e.score >> e.seconds >> std::quoted(e.name))) continue;
        row >> std::ws;
        if (!row.eof() || e.score < 0 || e.score > 100000 || !std::isfinite(e.seconds) ||
            e.seconds < 0 || e.seconds > 10000000) continue;
        e.name = cleanName(e.name);
        if (!e.name.empty()) entries_.push_back(e);
    }
    rankEntries(entries_);
}

bool Scoreboard::record(ScoreEntry entry) {
    entry.name = cleanName(entry.name);
    if (entry.name.empty() || entry.score < 0 || entry.score > 100000 ||
        !std::isfinite(entry.seconds) || entry.seconds < 0 || entry.seconds > 10000000) return false;
    entries_.push_back(entry);
    rankEntries(entries_);
    auto temp = path_;
    temp += ".tmp";
    std::ofstream file(temp, std::ios::trunc);
    for (const auto& e : entries_) file << e.score << ' ' << std::fixed << std::setprecision(3)
                                       << e.seconds << ' ' << std::quoted(e.name) << '\n';
    file.close();
    if (!file) return false;
    // Replace only after the complete new file has been closed successfully.
#ifdef _WIN32
    return MoveFileExW(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code error;
    std::filesystem::rename(temp, path_, error);
    return !error;
#endif
}
