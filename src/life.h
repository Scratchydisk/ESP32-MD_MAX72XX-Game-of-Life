#pragma once
#include <vector>

class GameOfLife
{
private:
    bool *board;
    bool *nextBoard;
    int width;
    int height;
    bool wrapAround;
    std::vector<unsigned long> boardHistory; // Store board hashes
    int maxHistorySize = 10;                 // Store last N states to detect oscillators
    unsigned int generationCount;
    unsigned int maxGenerations;

public:
    GameOfLife(int w, int h, bool wrap = true, unsigned int maxGen = 180);

    ~GameOfLife();

    void randomize();
    void computeNextGeneration();
    bool getCell(int x, int y) const;
    void setCell(int x, int y, bool state);
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isGameFinished();
    void resetGenerations() { generationCount = 0; }
    unsigned int getGenerationCount() const { return generationCount; }
    unsigned long calculateBoardHash() const;
    void clearHistory();
    void clear();

    // Additional methods for creating specific patterns
    void createGlider(int startX, int startY);
    void createBlinker(int startX, int startY);
    void createGliderGun(int startX, int startY);
    void createPulsar(int startX, int startY);

private:
    int countNeighbors(int x, int y) const;
    int getIndex(int x, int y) const;
};
