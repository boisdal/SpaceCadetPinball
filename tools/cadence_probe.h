// Opt-in audit instrumentation. No effect in normal builds.
#pragma once
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

class CadenceProbe
{
    using Clock = std::chrono::steady_clock;
    struct Event { double seconds; char kind; double value; };
    std::vector<Event> events;
    std::string path;
    Clock::time_point start{}, nextPoll{};
    bool started = false, done = false;
    int previousState = -1;
public:
    CadenceProbe()
    {
        const char* value = std::getenv("PINBALL_CADENCE_CSV");
        if (value && *value)
        {
            path = value;
            events.reserve(40000);
        }
    }
    void Record(char kind, double value)
    {
        if (path.empty() || done) return;
        auto now = Clock::now();
        if (!started)
        {
            if (now < nextPoll) return;
            nextPoll = now + std::chrono::seconds(1);
            auto trigger = std::fopen((path + ".start").c_str(), "r");
            if (!trigger) return;
            std::fclose(trigger);
            started = true;
            start = now;
            previousState = -1;
        }
        double elapsed = std::chrono::duration<double>(now - start).count();
        if (elapsed >= 45.0 || events.size() >= 40000)
        {
            done = true;
            auto file = std::fopen(path.c_str(), "w");
            if (!file) { std::perror("cadence output"); return; }
            std::fprintf(file, "seconds,event,value\n");
            for (auto& event : events)
                std::fprintf(file, "%.9f,%c,%.9f\n", event.seconds, event.kind, event.value);
            std::fprintf(file, "%.9f,E,%d\n", elapsed, elapsed >= 45.0 ? 1 : 0);
            std::fclose(file);
            std::printf("Cadence capture written: %s\n", path.c_str());
            return;
        }
        // State is 1 only when the main loop is focused and not paused.
        if (kind == 'S')
        {
            if (previousState == static_cast<int>(value)) return;
            previousState = static_cast<int>(value);
        }
        events.push_back({elapsed, kind, value});
    }
};
