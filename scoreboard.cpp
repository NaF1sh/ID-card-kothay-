#include "game.h"

#define SCORE_FILE "scoreboard.txt"
#define TEMP_SCORE_FILE "scoreboard.txt.tmp"
#define MAX_NAME_LENGTH 18
#define TOP_SCORES 10

static bool isNameChar(char c)
{
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;
    if (c >= '0' && c <= '9')
        return true;
    return c == ' ' || c == '-' || c == '_';
}

static void trimSpaces(char *text)
{
    int length = strlen(text);
    while (length > 0 && text[length - 1] == ' ')
    {
        length--;
        text[length] = '\0';
    }

    int firstLetter = 0;
    while (text[firstLetter] == ' ')
        firstLetter++;
    if (firstLetter > 0)
        memmove(text, text + firstLetter, length - firstLetter + 1);
}

void cleanName(const char *in, char *out)
{
    int length = 0;
    for (int i = 0; in[i] != '\0' && length < MAX_NAME_LENGTH; i++)
    {
        if (isNameChar(in[i]))
        {
            out[length] = in[i];
            length++;
        }
    }
    out[length] = '\0';
    trimSpaces(out);
}

static bool betterScore(ScoreEntry a, ScoreEntry b)
{
    if (a.score != b.score)
        return a.score > b.score;
    return a.seconds < b.seconds;
}

static void rankScores()
{
    for (int i = 1; i < scoreCount; i++)
    {
        ScoreEntry entry = scores[i];
        int j = i - 1;
        while (j >= 0 && betterScore(entry, scores[j]))
        {
            scores[j + 1] = scores[j];
            j--;
        }
        scores[j + 1] = entry;
    }
    if (scoreCount > TOP_SCORES)
        scoreCount = TOP_SCORES;
}

static bool validEntry(int score, float seconds)
{
    if (score < 0 || score > 100000)
        return false;
    if (isnan(seconds))
        return false;
    return seconds >= 0 && seconds <= 10000000;
}

static void addScore(const char *name, int score, float seconds)
{
    strcpy(scores[scoreCount].name, name);
    scores[scoreCount].score = score;
    scores[scoreCount].seconds = seconds;
    scoreCount++;
    rankScores();
}

void loadScores()
{
    scoreCount = 0;
    FILE *file = fopen(SCORE_FILE, "r");
    if (file == NULL)
        return;

    char line[200];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        int score;
        float seconds;
        char rawName[100];
        if (sscanf(line, "%d %f \"%99[^\"]\"", &score, &seconds, rawName) != 3)
            continue;
        if (!validEntry(score, seconds))
            continue;

        char name[20];
        cleanName(rawName, name);
        if (name[0] != '\0')
            addScore(name, score, seconds);
    }
    fclose(file);
}

static bool saveScores()
{
    FILE *file = fopen(TEMP_SCORE_FILE, "w");
    if (file == NULL)
        return false;
    for (int i = 0; i < scoreCount; i++)
        fprintf(file, "%d %.3f \"%s\"\n", scores[i].score, scores[i].seconds, scores[i].name);
    if (fclose(file) != 0)
        return false;

    remove(SCORE_FILE);
    return rename(TEMP_SCORE_FILE, SCORE_FILE) == 0;
}

bool recordScore(const char *name, int score, float seconds)
{
    char cleanedName[20];
    cleanName(name, cleanedName);
    if (cleanedName[0] == '\0' || !validEntry(score, seconds))
        return false;

    addScore(cleanedName, score, seconds);
    return saveScores();
}
