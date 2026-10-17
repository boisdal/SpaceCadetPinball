#include "../SpaceCadetPinball/DisplayLayout.h"
#include <cassert>
#include <cmath>
int main()
{
    const int sizes[][2] = {{600,416}, {752,520}, {960,666}};
    const int windows[][2] = {{640,480}, {1280,720}, {3840,2160}, {2160,3840}, {320,240}};
    for (auto& table : sizes) for (auto& window : windows)
    for (int menu : {19, 29, 38, 57}) for (bool integer : {false, true})
    {
        auto r = display_layout::fit(window[0], window[1], menu, table[0], table[1], true, integer);
        assert(r.scaleX == r.scaleY);
        assert(r.x >= 0 && r.y >= menu && r.width > 0 && r.height > 0);
        assert(r.x + r.width <= window[0] && r.y + r.height <= window[1]);
        assert(std::abs(float(r.width) / table[0] - float(r.height) / table[1]) < 0.004f);
        if (integer && r.scaleX >= 1) assert(r.scaleX == std::floor(r.scaleX));
    }
    auto tiny = display_layout::fit(0, 0, 40, 600, 416, true, false);
    assert(tiny.width > 0 && tiny.height > 0 && tiny.scaleY > 0);
}
