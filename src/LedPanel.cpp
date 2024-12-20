#include "LedPanel.h"

void LedPanel::drawPoint(int x, int y, bool on)
{
  // Validate x and y are within totalWidth and totalHeight
  if (x < 0 || x >= pixelWidth || y < 0 || y >= pixelHeight)
    return;

  // Determine which module (device) this point is in
  int moduleX = x / 8; // Module column index
  int moduleY = y / 8; // Module row index

  // Local position within the module
  int localX = x % 8;
  int localY = y % 8;

  // Adjust localX and localY based on module orientation
  // For FC16_HW, pixels within a module are reversed in the X direction
  localX = 7 - localX;

  // If rows are reversed within the module, adjust localY
  // Uncomment if needed based on your hardware orientation
  localY = 7 - localY;

  // Calculate the module index based on devices arranged from right to left, bottom to top
  // Adjust if your modules are arranged differently
  int moduleIndex = (devicesHigh - 1 - moduleY) * devicesWide + (devicesWide - 1 - moduleX);

  // Calculate the column index in the overall display
  int columnIndex = moduleIndex * 8 + localX;

  // Set the point on the display
  mx.setPoint(localY, columnIndex, on);
}

void LedPanel::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true)
  {
    drawPoint(x0, y0, true);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void LedPanel::spiral(bool inward)
{
  int left = 0, right = pixelWidth - 1;
  int top = 0, bottom = pixelHeight - 1;

  while ((inward && left <= right && top <= bottom) ||
         (!inward && left >= -1 && right < pixelWidth + 1 &&
          top >= -1 && bottom < pixelHeight + 1))
  {
    mx.clear();
    // Draw current spiral frame
    for (int i = left; i <= right; i++)
      drawPoint(i, inward ? top : bottom, true);
    for (int i = top; i <= bottom; i++)
      drawPoint(inward ? right : left, i, true);
    for (int i = right; i >= left; i--)
      drawPoint(i, inward ? bottom : top, true);
    for (int i = bottom; i >= top; i--)
      drawPoint(inward ? left : right, i, true);

    delay(100);

    if (inward)
    {
      left++;
      right--;
      top++;
      bottom--;
    }
    else
    {
      left--;
      right++;
      top--;
      bottom++;
    }
  }
}

void LedPanel::wave()
{
  int centerX = pixelWidth / 2;
  int centerY = pixelHeight / 2;

  for (int radius = 0; radius <= max(pixelWidth, pixelHeight); radius++)
  {
    mx.clear();
    for (int y = 0; y < pixelHeight; y++)
    {
      for (int x = 0; x < pixelWidth; x++)
      {
        int distance = sqrt((x - centerX) * (x - centerX) +
                            (y - centerY) * (y - centerY));
        drawPoint(x, y, (int)distance == radius);
      }
    }
    delay(100);
  }
}

void LedPanel::flash()
{
  // Blink current pattern 3 times
  for (int i = 0; i < 3; i++)
  {
    mx.transform(MD_MAX72XX::TINV);
    delay(200);
    mx.transform(MD_MAX72XX::TINV);
    delay(200);
  }
}