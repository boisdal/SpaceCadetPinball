// Test doubles expose controller events; method bodies come from production files.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

enum class MessageCode { ControlCollision, Other, TLightFlasherStartTimedThenStayOn,
    TLightGroupGetOnCount, TLightFlasherStartTimedThenStayOff };
struct vector2 { float X, Y; };
struct vector3 { float X, Y, Z; operator vector2() const { return {X, Y}; } };
constexpr float Pi = 3.14159265358979323846f;
typedef short Sint16;
typedef unsigned char Uint8;
struct TPinballComponent
{
    unsigned MessageField = 0;
    bool On = false, FlasherOnFlag = false;
    int starts = 0, resets = 0, count = 0;
    int Message(MessageCode code, float)
    {
        if (code == MessageCode::TLightFlasherStartTimedThenStayOn) { On = true; ++starts; }
        if (code == MessageCode::TLightFlasherStartTimedThenStayOff) ++resets;
        return count;
    }
    bool light_on() { return On; }
    int get_scoring(int) { return 500; }
    vector2 get_coordinates() { return {0, 0}; }
};
struct Table { int score = 0; void AddScore(int value) { score += value; } } table;
Table* TableG = &table;
TPinballComponent t13, t14, t15, l101, l102, l103, l198, bank;
TPinballComponent *target13 = &t13, *target14 = &t14, *lite101 = &l101,
    *lite102 = &l102, *lite103 = &l103, *lite198 = &l198, *ramp_tgt_lights = &bank;
namespace pb { bool FullTiltMode = false; int time_ticks = 0; }
struct TSound
{
    int calls = 0;
    TPinballComponent* caller = nullptr;
    void Play(TPinballComponent* source, const char* info)
    { ++calls; caller = source; assert(!strcmp(info, "MissionSpotTargetControl")); }
} ordinary, special;
TSound *soundwave49D = &ordinary, *soundwave52 = &special;
struct control { static void MissionSpotTargetControl(MessageCode, TPinballComponent*); };

struct Mix_Chunk {};
int mixerCalls = 0;
int Mix_Playing(int) { ++mixerCalls; return 0; }
int Mix_HaltChannel(int) { ++mixerCalls; return 0; }
int Mix_PlayChannel(int, Mix_Chunk*, int) { ++mixerCalls; return -1; }
void Mix_SetPosition(int, Sint16, Uint8) { ++mixerCalls; }
Mix_Chunk* Mix_LoadWAV(const char*) { ++mixerCalls; return nullptr; }
FILE* fopenu(const char* path, const char* mode) { return fopen(path, mode); }
namespace maths {
vector3 vector_sub(vector3 a, vector3 b) { return {a.X-b.X, a.Y-b.Y, a.Z-b.Z}; }
float magnitude(vector3 a) { return sqrtf(a.X*a.X + a.Y*a.Y + a.Z*a.Z); }
}
namespace options { struct { bool SoundStereo = false; } Options; }
struct ChannelInfo { int TimeStamp; vector2 Position; };
struct Sound {
    static bool MixOpen, enabled_flag;
    static int num_channels;
    static std::vector<ChannelInfo> Channels;
    static void PlaySound(Mix_Chunk*, int, TPinballComponent*, const char*);
    static Mix_Chunk* LoadWaveFile(const std::string&);
};
bool Sound::MixOpen = true, Sound::enabled_flag = true;
int Sound::num_channels = 8;
std::vector<ChannelInfo> Sound::Channels(8);
struct SoundEntry { Mix_Chunk* WavePtr; float Duration; };
struct loader {
    static SoundEntry sound_list[2];
    static float play_sound(int, TPinballComponent*, const char*);
};
SoundEntry loader::sound_list[2] = {};

// ACTUAL_METHODS

int main()
{
    int cases = 0;
    // Lamp count after the hit: cover every target, both modes, and every
    // combination of Windows mission-lamp on/flashing states.
    for (bool ft : {false, true})
    for (int lamps : {1, 2, 3})
    for (bool missionOn : {false, true})
    for (bool flashing : {false, true})
    for (int target : {0, 1, 2})
    {
        l101 = {}; l102 = {}; l103 = {}; l198 = {}; bank = {};
        ordinary = {}; special = {}; table = {};
        pb::FullTiltMode = ft;
        l198.On = missionOn; l198.FlasherOnFlag = flashing; bank.count = lamps;
        TPinballComponent* targets[] = {&t13, &t14, &t15};
        TPinballComponent* lampsByTarget[] = {&l101, &l102, &l103};
        control::MissionSpotTargetControl(MessageCode::Other, targets[target]);
        assert(table.score == 0 && ordinary.calls + special.calls == 0);
        control::MissionSpotTargetControl(MessageCode::ControlCollision, targets[target]);
        const bool expectedSpecial = ft ? lamps == 3 : !missionOn || flashing;
        assert(special.calls == int(expectedSpecial));
        assert(ordinary.calls == int(!expectedSpecial));
        assert((expectedSpecial ? special : ordinary).caller == targets[target]);
        assert(l101.MessageField == (1u << target));
        assert(lampsByTarget[target]->starts == 1);
        assert(bank.resets == int(lamps == 3));
        assert(table.score == 500);
        ++cases;
    }
    // Actual file-open, loader forwarding, and mixer guard: a missing WAV is
    // represented by nullptr and must never reach any SDL_mixer operation.
    auto missing = Sound::LoadWaveFile("nonexistent-resource-test.wav");
    assert(missing == nullptr && mixerCalls == 0);
    loader::sound_list[1] = {missing, -1};
    assert(loader::play_sound(1, &t13, "missing") == -1);
    assert(loader::play_sound(0, &t13, "absent") == 0);
    assert(loader::play_sound(-1, &t13, "invalid") == 0);
    assert(mixerCalls == 0);
    puts("PASS: 72 controller cases; missing WAV and absent sound IDs bypass mixer safely.");
    assert(cases == 72);
}
