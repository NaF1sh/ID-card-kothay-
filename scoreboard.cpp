// ============================================================
// scoreboard.cpp
// Top 10 runs saved in scoreboard.txt, one per line:
//   score seconds "name"
// Highest score first; the faster time wins a tie.
// ============================================================

#include "game.h"

#define SCORE_FILE "scoreboard.txt"

// keeps letters, digits, space, - and _ (max 18), trims spaces
void cleanName(const char *in, char *out)
{
    int n = 0;
    for (int i = 0; in[i] != '\0' && n < 18; i++)
    {
        char c = in[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_')
            out[n++] = c;
    }
    out[n] = '\0';

    // trim spaces at the end, then at the start
    while (n > 0 && out[n - 1] == ' ')
        out[--n] = '\0';
    int start = 0;
    while (out[start] == ' ')
        start++;
    if (start > 0)
        memmove(out, out + start, n - start + 1);
}

// true if a should be ranked above b
bool betterScore(ScoreEntry a, ScoreEntry b)
{
    if (a.score != b.score)
        return a.score > b.score;
    return a.seconds < b.seconds;
}

// simple insertion sort, then keep the best 10
void rankScores()
{
    for (int i = 1; i < scoreCount; i++)
    {
        ScoreEntry key = scores[i];
        int j = i - 1;
        while (j >= 0 && betterScore(key, scores[j]))
        {
            scores[j + 1] = scores[j];
            j--;
        }
        scores[j + 1] = key;
    }
    if (scoreCount > 10)
        scoreCount = 10;
}

bool validEntry(int score, float seconds)
{
    return score >= 0 && score <= 100000 && seconds >= 0 && seconds <= 10000000 && seconds == seconds;
}

void loadScores()
{
    scoreCount = 0;
    FILE *f = fopen(SCORE_FILE, "r");
    if (f == NULL)
        return;

    char line[200];
    while (fgets(line, sizeof(line), f) != NULL)
    {
        int score;
        float seconds;
        char raw[100];
        // the name is inside quotes and may contain spaces
        if (sscanf(line, "%d %f \"%99[^\"]\"", &score, &seconds, raw) != 3)
            continue;
        if (!validEntry(score, seconds))
            continue;
        char name[20];
        cleanName(raw, name);
        if (name[0] == '\0')
            continue;

        // keep at most 11 while reading: add, sort, cut back to 10
        strcpy(scores[scoreCount].name, name);
        scores[scoreCount].score = score;
        scores[scoreCount].seconds = seconds;
        scoreCount++;
        rankScores();
    }
    fclose(f);
}

bool recordScore(const char *name, int score, float seconds)
{
    char clean[20];
    cleanName(name, clean);
    if (clean[0] == '\0' || !validEntry(score, seconds))
        return false;

    strcpy(scores[scoreCount].name, clean);
    scores[scoreCount].score = score;
    scores[scoreCount].seconds = seconds;
    scoreCount++;
    rankScores();

    // write to a temporary file first, then replace the real one,
    // so a crash never leaves a half-written scoreboard
    FILE *f = fopen(SCORE_FILE ".tmp", "w");
    if (f == NULL)
        return false;
    for (int i = 0; i < scoreCount; i++)
        fprintf(f, "%d %.3f \"%s\"\n", scores[i].score, scores[i].seconds, scores[i].name);
    bool ok = (fclose(f) == 0);
    if (!ok)
        return false;
    remove(SCORE_FILE); // Windows rename() fails if the file exists
    return rename(SCORE_FILE ".tmp", SCORE_FILE) == 0;
}
