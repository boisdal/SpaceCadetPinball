// Audit-only integration scenario; uses the loaded table and real timers/components.
#pragma once
#include "../SpaceCadetPinball/control.h"
#include "../SpaceCadetPinball/TSink.h"
#include "../SpaceCadetPinball/TDrain.h"
#include "../SpaceCadetPinball/TLight.h"
#include "../SpaceCadetPinball/TKickout.h"
#include "../SpaceCadetPinball/THole.h"
#include "../SpaceCadetPinball/nudge.h"

class MultiballReplay
{
    unsigned event = 0;
    FILE* log = nullptr;
    TKickout* kick = nullptr;
    THole* hole = nullptr;
    TBall* captured = nullptr;
    static void Require(bool condition, const char* message)
    {
        if (!condition) { std::fprintf(stderr, "Multiball audit FAILED: %s\n", message); std::exit(4); }
    }
    static TPinballComponent* Component(const char* name)
    {
        for (auto c : pb::MainTable->ComponentList)
            if (c->GroupName && std::strcmp(c->GroupName, name) == 0) return c;
        Require(false, name); return nullptr;
    }
    static TBall* ActiveBall()
    {
        for (auto b : pb::MainTable->BallList) if (b->ActiveFlag) return b;
        Require(false, "active ball required"); return nullptr;
    }
    static int ActiveCount()
    {
        int n=0; for (auto b : pb::MainTable->BallList) n += b->ActiveFlag ? 1 : 0; return n;
    }
    static void Lock(int expected)
    {
        auto table = pb::MainTable;
        Require(table->MultiballCount == 1 && ActiveCount() == 1, "one real ball before lock");
        Component("lite4")->MessageField = 1; // Controlled eligibility; collision and all consequences are real.
        auto sink = dynamic_cast<TSink*>(Component("v_sink1"));
        Require(sink != nullptr, "sink1 type");
        auto ball = ActiveBall();
        auto position = sink->BallPosition;
        auto direction = vector2{0,0};
        sink->Collision(ball, &position, &direction, 0, nullptr);
        Require(!ball->ActiveFlag, "sink disables captured ball");
        Require(table->BallLockedCounter == expected, "lock counter after real sink collision");
        Require(table->MultiballCount == (expected ? 0 : 3), "reserved count after lock");
    }
    static void Drain(int expected)
    {
        auto table=pb::MainTable;
        auto ball=ActiveBall();
        vector2 position{}, direction{};
        table->Drain->Collision(ball,&position,&direction,0,nullptr);
        Require(!ball->ActiveFlag, "drain disables ball");
        Require(table->MultiballCount==expected && ActiveCount()==expected, "real drain decrements active count");
    }
public:
    bool Enabled = std::getenv("PINBALL_AUDIT_MULTIBALL") != nullptr;
    ~MultiballReplay() {if(log) std::fclose(log);}
    void BeforeStep(double simulated)
    {
        if (!Enabled) return;
        Require(pb::FullTiltMode, "Full Tilt resources required");
        const double times[] = {4000, 7000, 10000, 12100, 12200, 12300, 12400,
            12400 + pb::MainTable->Drain->TimerTime * 1000 + 960 + 200,
            16000, 16050, 16200, 16400, 17500};
        if(event>=sizeof(times)/sizeof(times[0]) || simulated+0.0001<times[event]) return;
        auto table=pb::MainTable;
        switch(event)
        {
        case 0: Lock(1); break;
        case 1: Lock(2); break;
        case 2: Lock(0); break;
        case 3:
        {
            Require(table->MultiballCount==3 && ActiveCount()==3, "three actual balls after sink timers");
            std::vector<vector2> velocities;
            for(auto ball:table->BallList) if(ball->ActiveFlag)
            {
                Require(std::isfinite(ball->Speed) && ball->Speed>0, "finite nonzero ejection speed");
                Require(!ball->CollisionComp,"ejected balls free for nudge");
                velocities.push_back({ball->Direction.X*ball->Speed,ball->Direction.Y*ball->Speed});
            }
            nudge::nudge_right();
            Require(nudge::nudged_right!=0, "real nudge engages");
            unsigned i=0;
            for(auto ball:table->BallList) if(ball->ActiveFlag)
            {
                Require(std::abs(ball->Direction.X*ball->Speed-velocities[i].X-1.0f)<0.0001f &&
                    std::abs(ball->Direction.Y*ball->Speed-velocities[i].Y-0.5f)<0.0001f,
                    "nudge changes all three actual velocities");
                ++i;
            }
            break;
        }
        case 4:
            Component("lite199")->Message(MessageCode::TLightResetAndTurnOff,0);
            Component("lite200")->Message(MessageCode::TLightResetAndTurnOff,0);
            Drain(2); Require(table->BallCount==3, "first multiball drain keeps reserve"); break;
        case 5: Drain(1); Require(table->BallCount==3, "return to one ball keeps reserve"); break;
        case 6: Drain(0); Require(table->BallCount==2, "last drain consumes reserve"); break;
        case 7:
            Require(table->MultiballCount==1 && ActiveCount()==1, "real drain/feed timers restore single ball");
            Require(!nudge::nudged_right, "real un-nudge timer ran");
            std::puts("PASS: loaded FT table, real sink locks/replacement timers/three ejections/nudge/drains/feed");
            break;
        case 8:
            for(auto c:table->ComponentList) if((kick=dynamic_cast<TKickout*>(c))) break;
            Require(kick!=nullptr,"loaded kickout");
            captured=ActiveBall();
            {
                vector2 p{},d{};
                table->TiltLockFlag=1; // Select the actual short emergency-ejection path.
                kick->Collision(captured,&p,&d,0,nullptr);
                table->TiltLockFlag=0;
            }
            Require(kick->BallCaputeredFlag && captured->CollisionComp==kick,"kickout captures actual ball");
            break;
        case 9: Require(kick->BallCaputeredFlag && captured->CollisionComp==kick,"kickout waits for timer"); break;
        case 10:
            Require(!kick->BallCaputeredFlag && !captured->CollisionComp && std::isfinite(captured->Speed) && captured->Speed>0,
                "kickout timer ejects actual ball");
            break;
        case 11:
            for(auto c:table->ComponentList) if((hole=dynamic_cast<THole*>(c))) break;
            Require(hole!=nullptr,"loaded hole");
            captured=ActiveBall(); captured->Position.Z=hole->ZSetValue+2;
            {
                vector2 p{},d{};
                hole->Collision(captured,&p,&d,0,nullptr);
            }
            Require(hole->BallCapturedFlag && captured->CollisionComp==hole,"hole captures actual ball");
            break;
        case 12:
            Require(!hole->BallCapturedFlag && captured->CollisionComp!=hole,"hole timer/gravity releases actual ball");
            Require(captured->CollisionMask==hole->CollisionMask,"hole changes collision layer");
            std::puts("PASS: loaded kickout delayed capture/ejection and hole timer/gravity/layer release");
            break;
        }
        std::printf("Multiball event %u at %.3f ms: active=%d reserved=%d locks=%d reserve=%d\n",
            event++,simulated,ActiveCount(),table->MultiballCount,table->BallLockedCounter,table->BallCount);
    }
    void Validate() {if(Enabled) Require(event==13,"all integration stages completed");}
    void AfterStep(double simulated)
    {
        if(!Enabled) return;
        if(!log)
        {
            const char* path=std::getenv("PINBALL_CADENCE_CSV"); Require(path!=nullptr,"output path");
            log=std::fopen((std::string(path)+".multiball.csv").c_str(),"w"); Require(log!=nullptr,"log open");
            std::fprintf(log,"time_ms,event,ball_id,active,x,y,z,speed,dx,dy,mask,captured,reserved,locks,reserve,nudged\n");
        }
        auto table=pb::MainTable; int id=0;
        for(auto b:table->BallList)
            std::fprintf(log,"%.6f,%u,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%d,%d,%d,%d\n",
                simulated,event,id++,b->ActiveFlag,b->Position.X,b->Position.Y,b->Position.Z,b->Speed,
                b->Direction.X,b->Direction.Y,b->CollisionMask,b->CollisionComp!=nullptr,
                table->MultiballCount,table->BallLockedCounter,table->BallCount,nudge::nudged_right);
    }
};
