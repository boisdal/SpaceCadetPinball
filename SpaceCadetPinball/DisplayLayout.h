#pragma once
#include <algorithm>
#include <cmath>

namespace display_layout
{
struct Layout { float scaleX, scaleY; int x, y, width, height; };
// Rendering only: table geometry and simulation stay in their original units.
inline Layout fit(int width, int height, int menuHeight, int tableWidth, int tableHeight,
                  bool uniform, bool integer)
{
    width = std::max(1, width);
    height = std::max(1, height - menuHeight);
    float sx = float(width) / tableWidth, sy = float(height) / tableHeight;
    if (integer) { if (sx >= 1) sx = std::floor(sx); if (sy >= 1) sy = std::floor(sy); }
    if (uniform) sx = sy = std::min(sx, sy);
    int spareX = std::max(0, int(std::floor(width - tableWidth * sx)));
    int spareY = std::max(0, int(std::floor(height - tableHeight * sy)));
    return {sx, sy, spareX / 2, spareY / 2 + menuHeight, width - spareX, height - spareY};
}
}
