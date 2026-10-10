#include "../SpaceCadetPinball/FrameScheduler.h"
#include <cassert>
#include <cmath>
#include <iostream>

int drain(FrameScheduler& s)
{
    int count = 0;
    while (count < FrameScheduler::MaxStepsPerPass && s.TakeStep()) ++count;
    return count;
}

int main()
{
    // Identical wall time yields identical physics, regardless of rendering.
    for (int fps : {60, 90, 120, 144, 360})
    {
        FrameScheduler s;
        s.Advance(0, true, 120, fps);
        int updates = 0, frames = 0;
        for (int ms = 0; ms < 10000; ++ms)
        {
            assert(s.Advance(1, true, 120, fps) == 0);
            updates += drain(s);
            if (s.TakeFrame()) ++frames;
        }
        assert(updates == 1200);
        assert(std::abs(frames - fps * 10) <= 1);
    }
    FrameScheduler s;
    s.Advance(0, true, 120, 60);
    assert(s.Advance(118, true, 120, 60) == 0);
    assert(drain(s) == 8); // bound per pass; poll inputs again before recovery
    assert(s.WaitMs() == 0);
    assert(drain(s) == 6); // all 14 steps retained, no 100 ms loss
    s.Advance(7, true, 120, 60);
    assert(drain(s) == 1); // fractional debt also retained
    assert(s.TakeFrame());
    assert(!s.TakeFrame()); // no presentation backlog burst
    s.Advance(1000, false, 120, 60);
    assert(drain(s) == 0);
    s.Advance(1000, true, 120, 60);
    assert(drain(s) == 0); // focus/pause time cannot leak into simulation
    assert(s.Advance(1000, true, 120, 60) == 750);
    int total = 0;
    while (int n = drain(s)) total += n;
    assert(total == 30); // bounded recovery after exceptional stall
    s.Advance(0, true, 240, 144);
    s.Advance(1000.0 / 240, true, 240, 144);
    assert(drain(s) == 1);
    assert(std::abs(s.StepMs() - 1000.0 / 240) < 1e-9);
    std::cout << "FrameScheduler: all checks passed\n";
}
