#pragma once

#include <MD_MAX72xx.h>

// Graphics library for MD_MAX72XX
// E.g. for 4 devices
// [3][2][1][0] <= Microcontroller
// Arranged as a 2 x 2 panel matrix, e.g.
// [1][0]
// [3][2]
// Origin is bottom left (i.e. panel 3)
//
// 8 devices, connected as 4 x 2 is
// [3][2][1][0]
// [7][6][5][4]
// Origin  is bottom left (i.e. panel 7)
class LedPanel
{
private:
    MD_MAX72XX &mx;
    uint8_t devicesWide;  // Number of devices in the horizontal direction
    uint8_t devicesHigh;  // Number of devices in the vertical direction
    uint16_t pixelWidth;  // Total display width in pixels
    uint16_t pixelHeight; // Total display height in pixels

public:
    uint16_t width() const { return pixelWidth; }
    uint16_t height() const { return pixelHeight; }

    // Constructor accepts the matrix object and device arrangement
    LedPanel(MD_MAX72XX &matrix, uint8_t devWide, uint8_t devHigh)
        : mx(matrix), devicesWide(devWide), devicesHigh(devHigh)
    {
        pixelWidth = devicesWide * 8;
        pixelHeight = devicesHigh * 8;
    }

    // Basic graphical functions
    void drawPoint(int x, int y, bool on);
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

    // More advanced graphics functions
   
    void spiral(bool inward);
    void wave();
    void flash();
};
