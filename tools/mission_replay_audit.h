// Audit-only integration checks: real table, lamps, text queues, drain and timers.
#pragma once
#include "../SpaceCadetPinball/control.h"
#include "../SpaceCadetPinball/TLight.h"
#include "../SpaceCadetPinball/TTextBox.h"
#include "../SpaceCadetPinball/TDrain.h"
#include "../SpaceCadetPinball/translations.h"

class MissionReplayAudit
{
    int phase = 0;
    int originalBalls = 0;
    static TLight* light(const char* name)
    {
        return dynamic_cast<TLight*>(pb::MainTable->find_component(name));
    }
    static void check(bool passed, const char* name)
    {
        std::printf("mission_replay:%s:%s\n", passed ? "PASS" : "FAIL", name);
        std::fflush(stdout);
        if (!passed) std::exit(23);
    }
    static bool text(Msg id)
    {
        for (auto m = pb::InfoTextBox->CurrentMessage; m; m = m->NextMessage)
            if (std::strcmp(m->Text, pb::get_rc_string(id)) == 0) return true;
        return false;
    }
    static void drain()
    {
        auto table = pb::MainTable;
        auto ball = table->BallList.front();
        check(ball->ActiveFlag != 0, "ball active before drain");
        auto d = dynamic_cast<TDrain*>(table->find_component("drain"));
        check(d != nullptr, "real drain component");
        d->Collision(ball, nullptr, nullptr, 0.0f, nullptr);
    }
public:
    bool Enabled = std::getenv("PINBALL_AUDIT_MISSION_REPLAY") != nullptr;
    void BeforeStep(double ms)
    {
        if (!Enabled) return;
        auto table = pb::MainTable;
        if (phase == 0)
        {
            table->Message(MessageCode::NewGame, 2);
            table->Message(MessageCode::NewGame, 2);
            originalBalls = table->BallCount;
            ++phase;
        }
        if (phase == 1 && ms >= 4000)
        {
            check(table->PlayerCount == 2 && table->CurrentPlayer == 0, "two-player game initialized");
            pb::InfoTextBox->Clear();
            light("lite198")->MessageField = 2;
            control::MissionControl(MessageCode::ControlMissionComplete, nullptr);
            check(!light("lite199")->light_on(), "mission start gives no replay");
            // Set only the mission's remaining hit counter, then route a real event.
            light("lite56")->MessageField = 1;
            control::MissionControl(MessageCode::ControlCollision, table->find_component("a_bump1"));
            check(light("lite199")->light_on() == pb::FullTiltMode, "terminal mission lamp matches mode");
            check(text(Msg::STRING101) == pb::FullTiltMode, "terminal mission message matches mode");
            if (!pb::FullTiltMode) control::table_set_replay(-1.0f); // Existing XP replay, for lifecycle checks.
            table->Message(MessageCode::SwitchToNextPlayer, 0);
            check(table->CurrentPlayer == 1 && !light("lite199")->light_on(), "player two does not inherit replay");
            table->Message(MessageCode::SwitchToNextPlayer, 0);
            check(table->CurrentPlayer == 0 && light("lite199")->light_on(), "player one replay restored");
            ++phase;
        }
        if (phase == 2 && ms >= 5000)
        {
            light("lite200")->Message(MessageCode::TLightResetAndTurnOff, 0);
            pb::InfoTextBox->Clear();
            drain();
            check(!light("lite199")->light_on() && light("lite200")->light_on(), "drain consumes replay and arms ball save");
            check(table->BallCount == originalBalls && table->CurrentPlayer == 0, "replay preserves player and balls");
            check(text(Msg::STRING196), "replay ball message queued");
            ++phase;
        }
        if (phase == 3 && ms >= 9000)
        {
            check(table->BallList.front()->ActiveFlag != 0 && table->MultiballCount == 1 && !table->BallInDrainFlag,
                "real drain and plunger timers feed replay ball");
            control::table_set_replay(-1.0f);
            table->tilt(pb::time_now);
            check(table->TiltLockFlag != 0, "real tilt lock active");
            drain();
            check(!light("lite199")->light_on(), "tilt cancels replay");
            check(table->CurrentPlayer == 1 && table->PlayerScores[0].BallCount == originalBalls - 1,
                "tilted loss decrements player one and switches to player two");
            ++phase;
        }
        if (phase == 4 && ms >= 14000)
        {
            check(table->BallList.front()->ActiveFlag != 0 && !table->BallInDrainFlag,
                "real timers feed next player ball");
            check(table->CurrentPlayer == 1 && !light("lite199")->light_on(), "next player has no replay");
            std::puts("mission_replay:COMPLETE");
            ++phase;
        }
    }
};
