// Audit-only, controlled inputs at simulation times, never a shipping input path.
#pragma once
#include "../SpaceCadetPinball/TPinballTable.h"
#include "../SpaceCadetPinball/TBall.h"
#include "../SpaceCadetPinball/TFlipper.h"
#include "../SpaceCadetPinball/TFlipperEdge.h"
#include "mission_replay_audit.h"
#include "multiball_replay.h"
#include "../SpaceCadetPinball/TPlunger.h"

class CadenceReplay
{
    MissionReplayAudit missionReplay;
    MultiballReplay multiball;
    struct Sample { double time; float x, y, z, speed, left, right; int score, active, balls; };
    std::vector<Sample> samples;
    std::chrono::steady_clock::time_point created = std::chrono::steady_clock::now();
    double simulated = 0;
    unsigned nextInput = 0;
    bool started = false, stalled = false;
    struct PhysicsSample { double time; float dx, dy, boost, threshold; };
    std::vector<PhysicsSample> physicsSamples;
    const char* physicsScenario = std::getenv("PINBALL_PHYSICS_SCENARIO");
public:
    bool Enabled = std::getenv("PINBALL_AUDIT_REPLAY") != nullptr;
    static int Env(const char* name, int fallback)
    {
        const char* value = std::getenv(name);
        return value ? std::atoi(value) : fallback;
    }
    CadenceReplay()
    {
        if (!Enabled) return;
        samples.reserve(10000);
        if (physicsScenario && std::strcmp(physicsScenario, "weak") &&
            std::strcmp(physicsScenario, "maximum") && std::strcmp(physicsScenario, "automatic"))
        {
            std::fprintf(stderr, "Unknown physics scenario\n");
            std::exit(3);
        }
        if (physicsScenario) physicsSamples.reserve(10000);
        options::Options.FramesPerSecond = Env("PINBALL_AUDIT_FPS", 60);
        options::Options.UpdatesPerSecond = 120;
        options::Options.UncappedUpdatesPerSecond = false;
        winmain::UpdateFrameRate();
        SDL_SetWindowSize(winmain::MainWindow, Env("PINBALL_AUDIT_WIDTH", 800), Env("PINBALL_AUDIT_HEIGHT", 600));
        int windowW, windowH, outputW, outputH;
        SDL_GetWindowSize(winmain::MainWindow, &windowW, &windowH);
        SDL_GetRendererOutputSize(winmain::Renderer, &outputW, &outputH);
        float dpi = 0;
        SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(winmain::MainWindow), &dpi, nullptr, nullptr);
        std::printf("Audit: window=%dx%d output=%dx%d display_dpi=%.2f FPS=%d UPS=120\n",
            windowW, windowH, outputW, outputH, dpi, options::Options.FramesPerSecond.V);
        std::srand(1);
        // A second New Game skips the startup light show, just like the menu.
        pb::replay_level(false);
    }
    bool Ready(CadenceProbe& probe)
    {
        if (!Enabled) return true;
        if (!started)
        {
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - created).count() < 0.5) return false;
            started = true;
            probe.Start();
        }
        return true;
    }
    void BeforeStep()
    {
        if (!Enabled) return;
        if (multiball.Enabled) { multiball.BeforeStep(simulated); return; }
        if (missionReplay.Enabled) { missionReplay.BeforeStep(simulated); return; }
        if (physicsScenario)
        {
            auto plunger = pb::MainTable->Plunger;
            if (nextInput == 0 && simulated + 0.0001 >= 4000)
            {
                plunger->Message(std::strcmp(physicsScenario, "automatic") == 0 ?
                    MessageCode::PlungerLaunchBall : MessageCode::PlungerInputPressed, 0.0f);
                CadenceProbe::Active()->Record('K', nextInput++);
            }
            const auto release = std::strcmp(physicsScenario, "weak") == 0 ? 4100 : 7000;
            if (nextInput == 1 && std::strcmp(physicsScenario, "automatic") && simulated + 0.0001 >= release)
            {
                plunger->Message(MessageCode::PlungerInputReleased, 0.0f);
                CadenceProbe::Active()->Record('K', nextInput++);
            }
            return;
        }
        // Launch, then alternate and simultaneous flippers. Repeat every 2 s.
        // Allow the feed timer and the ball's bounces on the plunger to settle.
        while (nextInput < 25 && simulated + 0.0001 >= 4000 + nextInput * 500)
        {
            if (nextInput == 0)
            {
                if (!pb::MainTable->BallList.front()->ActiveFlag)
                {
                    std::fprintf(stderr, "Audit launch rejected: no active ball\n");
                    std::exit(3);
                }
                pb::launch_ball();
            }
            else
            {
                auto phase = (nextInput - 1) % 4;
                auto left = options::Options.Key[~GameBindings::LeftFlipper].Inputs[0];
                auto right = options::Options.Key[~GameBindings::RightFlipper].Inputs[0];
                if (phase == 0 || phase == 2) pb::InputDown(left);
                if (phase == 1 || phase == 2) pb::InputDown(right);
                if (phase == 1 || phase == 3) pb::InputUp(left);
                if (phase == 0 || phase == 3) pb::InputUp(right);
            }
            CadenceProbe::Active()->Record('K', nextInput++);
        }
    }
    void AfterStep(float dt)
    {
        if (!Enabled) return;
        simulated += dt;
        multiball.AfterStep(simulated);
        auto table = pb::MainTable;
        auto ball = table->BallList.front();
        if (physicsScenario) physicsSamples.push_back({simulated, ball->Direction.X, ball->Direction.Y,
            table->Plunger->Boost, table->Plunger->Threshold});
        samples.push_back({simulated, ball->Position.X, ball->Position.Y, ball->Position.Z, ball->Speed,
            table->FlipperL->FlipperEdge->CurrentAngle, table->FlipperR->FlipperEdge->CurrentAngle,
            table->CurScore, ball->ActiveFlag, table->BallCount});
    }
    void Stall()
    {
        if (Enabled && !stalled && simulated >= 9000)
        {
            stalled = true;
            if (Env("PINBALL_AUDIT_RESIZE", 0))
            {
                SDL_SetWindowSize(winmain::MainWindow, 1100, 740);
                options::Options.FramesPerSecond = 144;
                winmain::UpdateFrameRate();
                winmain::ImIO->FontGlobalScale = 2.0f;
                CadenceProbe::Active()->Record('X', 144);
            }
            auto ms = Env("PINBALL_AUDIT_STALL_MS", 0);
            if (ms > 0) { CadenceProbe::Scope scope('J'); std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
        }
    }
    bool Finished() const { return Enabled && simulated >= 18000 - 0.001; }
    void Save()
    {
        if (!Enabled) return;
        multiball.Validate();
        auto path = std::getenv("PINBALL_CADENCE_CSV");
        if (!path) return;
        auto file = std::fopen((std::string(path) + ".trajectory.csv").c_str(), "w");
        if (!file) return;
        std::fprintf(file, "time_ms,x,y,z,speed,left,right,score,active,balls\n");
        for (auto& s : samples) std::fprintf(file, "%.9f,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d\n",
            s.time, s.x, s.y, s.z, s.speed, s.left, s.right, s.score, s.active, s.balls);
        std::fclose(file);
        if (physicsScenario)
        {
            file = std::fopen((std::string(path) + ".physics.csv").c_str(), "w");
            if (!file) return;
            std::fprintf(file, "time_ms,direction_x,direction_y,plunger_boost,plunger_threshold\n");
            for (auto& s : physicsSamples) std::fprintf(file, "%.9f,%.9g,%.9g,%.9g,%.9g\n",
                s.time, s.dx, s.dy, s.boost, s.threshold);
            std::fclose(file);
        }
    }
};
