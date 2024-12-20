
#include "life.h"
#include <stdlib.h>
#include <time.h>

GameOfLife::GameOfLife(int w, int h, bool wrap, unsigned int maxGen)
    : width(w), height(h), wrapAround(wrap),
      generationCount(0), maxGenerations(maxGen)
{
    board = new bool[width * height]();
    nextBoard = new bool[width * height]();
    srand(time(NULL));
}

GameOfLife::~GameOfLife()
{
    delete[] board;
    delete[] nextBoard;
}

void GameOfLife::randomize()
{
    for (int i = 0; i < width * height; i++)
    {
        board[i] = (rand() % 2) == 1;
    }
}

int GameOfLife::getIndex(int x, int y) const
{
    if (wrapAround)
    {
        x = (x + width) % width;
        y = (y + height) % height;
    }
    return y * width + x;
}

bool GameOfLife::getCell(int x, int y) const
{
    if (!wrapAround && (x < 0 || x >= width || y < 0 || y >= height))
        return false;
    return board[getIndex(x, y)];
}

void GameOfLife::setCell(int x, int y, bool state)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
        board[getIndex(x, y)] = state;
}

int GameOfLife::countNeighbors(int x, int y) const
{
    int count = 0;
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dy == 0)
                continue;
            if (getCell(x + dx, y + dy))
                count++;
        }
    }
    return count;
}

void GameOfLife::computeNextGeneration()
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int neighbors = countNeighbors(x, y);
            bool currentCell = getCell(x, y);

            nextBoard[getIndex(x, y)] =
                (currentCell && (neighbors == 2 || neighbors == 3)) ||
                (!currentCell && neighbors == 3);
        }
    }

    bool *temp = board;
    board = nextBoard;
    nextBoard = temp;
    generationCount++;
}

bool GameOfLife::isGameFinished()
{
    // Check generation limit first
    if (generationCount >= maxGenerations)
    {
        return true;
    }
    // First check if the board is static (no changes)
    bool isStatic = true;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int neighbors = countNeighbors(x, y);
            bool currentCell = getCell(x, y);
            bool nextState = (currentCell && (neighbors == 2 || neighbors == 3)) ||
                             (!currentCell && neighbors == 3);

            if (nextState != currentCell)
            {
                isStatic = false;
                break;
            }
        }
        if (!isStatic)
            break;
    }

    if (isStatic)
    {
        clearHistory();
        return true;
    }

    // Check for oscillating patterns
    unsigned long currentHash = calculateBoardHash();

    // Look for this hash in our history
    for (unsigned long previousHash : boardHistory)
    {
        if (previousHash == currentHash)
        {
            clearHistory();
            return true; // Pattern repeats
        }
    }

    // Add current state to history
    boardHistory.push_back(currentHash);
    if (boardHistory.size() > maxHistorySize)
    {
        boardHistory.erase(boardHistory.begin());
    }

    return false;
}

unsigned long GameOfLife::calculateBoardHash() const
{
    unsigned long hash = 5381; // Initial value (djb2 algorithm)
    for (int i = 0; i < width * height; i++)
    {
        hash = ((hash << 5) + hash) + (board[i] ? 1 : 0);
    }
    return hash;
}

void GameOfLife::clearHistory()
{
    boardHistory.clear();
}

void GameOfLife::createGlider(int startX, int startY)
{
    setCell(startX + 1, startY, true);     // .#.
    setCell(startX + 2, startY + 1, true); // ..#
    setCell(startX, startY + 2, true);     // ###
    setCell(startX + 1, startY + 2, true);
    setCell(startX + 2, startY + 2, true);
}

void GameOfLife::createBlinker(int startX, int startY)
{
    setCell(startX, startY, true); // ###
    setCell(startX + 1, startY, true);
    setCell(startX + 2, startY, true);
}

void GameOfLife::createPulsar(int startX, int startY)
{
    // Outer squares
    for (int i = 0; i < 3; i++)
    {
        setCell(startX + 2 + i, startY, true);
        setCell(startX + 2 + i, startY + 5, true);
        setCell(startX + 8 + i, startY, true);
        setCell(startX + 8 + i, startY + 5, true);

        setCell(startX, startY + 2 + i, true);
        setCell(startX + 5, startY + 2 + i, true);
        setCell(startX + 7, startY + 2 + i, true);
        setCell(startX + 12, startY + 2 + i, true);
    }
}

void GameOfLife::createGliderGun(int startX, int startY)
{
    // Left square
    setCell(startX + 1, startY + 5, true);
    setCell(startX + 2, startY + 5, true);
    setCell(startX + 1, startY + 6, true);
    setCell(startX + 2, startY + 6, true);

    // Left pattern
    setCell(startX + 11, startY + 5, true);
    setCell(startX + 11, startY + 6, true);
    setCell(startX + 11, startY + 7, true);
    setCell(startX + 12, startY + 4, true);
    setCell(startX + 12, startY + 8, true);
    setCell(startX + 13, startY + 3, true);
    setCell(startX + 13, startY + 9, true);
    setCell(startX + 14, startY + 3, true);
    setCell(startX + 14, startY + 9, true);
    setCell(startX + 15, startY + 6, true);
    setCell(startX + 16, startY + 4, true);
    setCell(startX + 16, startY + 8, true);
    setCell(startX + 17, startY + 5, true);
    setCell(startX + 17, startY + 6, true);
    setCell(startX + 17, startY + 7, true);
    setCell(startX + 18, startY + 6, true);

    // Right pattern
    setCell(startX + 21, startY + 3, true);
    setCell(startX + 21, startY + 4, true);
    setCell(startX + 21, startY + 5, true);
    setCell(startX + 22, startY + 3, true);
    setCell(startX + 22, startY + 4, true);
    setCell(startX + 22, startY + 5, true);
    setCell(startX + 23, startY + 2, true);
    setCell(startX + 23, startY + 6, true);
    setCell(startX + 25, startY + 1, true);
    setCell(startX + 25, startY + 2, true);
    setCell(startX + 25, startY + 6, true);
    setCell(startX + 25, startY + 7, true);

    // Right square
    setCell(startX + 35, startY + 3, true);
    setCell(startX + 35, startY + 4, true);
    setCell(startX + 36, startY + 3, true);
    setCell(startX + 36, startY + 4, true);
}

void GameOfLife::clear()
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            setCell(x, y, false);
        }
    }
}
