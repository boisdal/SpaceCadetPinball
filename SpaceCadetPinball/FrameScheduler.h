#pragma once
#include <algorithm>
#include <cmath>

// Wall time schedules rendering; only whole, fixed steps advance the game.
// Keep short stalls as debt, but bound recovery after a debugger/driver stall.
class FrameScheduler
{
    double pending = 0, renderIn = 0, step = 1000.0 / 120, period = 1000.0 / 60;
    bool running = false;
public:
    static constexpr int MaxStepsPerPass = 8;
    double Advance(double elapsedMs, bool active, int ups, int fps)
    {
        const double newStep = 1000.0 / ups, newPeriod = 1000.0 / fps;
        if (newStep != step || !active || !running) pending = 0;
        if (newPeriod != period) renderIn = 0;
        step = newStep;
        period = newPeriod;
        elapsedMs = std::max(0.0, elapsedMs);
        renderIn -= elapsedMs;
        double dropped = 0;
        if (active && running)
        {
            pending += elapsedMs;
            dropped = std::max(0.0, pending - 250.0);
            pending -= dropped;
        }
        running = active;
        return dropped;
    }
    bool TakeStep()
    {
        if (!running || pending + 1e-9 < step) return false;
        pending = std::max(0.0, pending - step);
        return true;
    }
    double StepMs() const { return step; }
    bool TakeFrame()
    {
        if (renderIn > 1e-9) return false;
        // Skip obsolete presentations, never burst-render a missed frame queue.
        renderIn = period - std::fmod(std::max(0.0, -renderIn), period);
        return true;
    }
    double WaitMs() const
    {
        return std::max(0.0, running ? std::min(step - pending, renderIn) : renderIn);
    }
};
