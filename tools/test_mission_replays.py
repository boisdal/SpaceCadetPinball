"""Compile production mission/drain functions with observable component doubles.

No assets, display, or copied controller implementation. This tests event routing,
not rendering, audio playback, timers, or the original executable.
Usage: python tools/test_mission_replays.py --cxx path/to/zig.exe
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# Expected terminal events, independently transcribed from the reference matrix.
FINALS = {
    'PracticeMission': 'bump1', 'LaunchTraining': 'ramp',
    'ReentryTraining': 'roll1', 'ScienceMission': 'target1',
    'StrayComet': 'kickout2', 'BlackHoleThreat': 'kickout3',
    'SpaceRadiation': 'sink1', 'BugHunt': 'target1',
    'RescueMission': 'kickout2', 'Satellite': 'bump4',
    'Reconnaissance': 'roll1', 'DoomsdayMachine': 'roll4',
    'AlienMenacePartTwo': 'bump1', 'CosmicPlaguePartTwo': 'roll9',
    'SecretMissionGreen': 'sink2', 'MaelstromPartEight': 'kickout2',
}
INTERMEDIATE = {
    'AlienMenace': 'bump1', 'CosmicPlague': 'flag1',
    'SecretMissionYellow': 'sink3', 'SecretMissionRed': 'sink1',
    'TimeWarp': 'rebo1', 'TimeWarpPartTwo': 'ramp',
    'Maelstrom': 'target1', 'MaelstromPartTwo': 'target10',
    'MaelstromPartThree': 'roll1', 'MaelstromPartFour': 'roll184',
    'MaelstromPartFive': 'ramp', 'MaelstromPartSix': 'flag1',
    'MaelstromPartSeven': 'sink1',
}


def generate():
    source = (ROOT / 'SpaceCadetPinball/control.cpp').read_text()
    names = [n + 'Controller' for n in [*FINALS, *INTERMEDIATE]]
    names += ['table_set_replay', 'BallDrainControl']
    functions = []
    for name in names:
        start = source.index('void control::' + name + '(')
        end = source.find('\nvoid control::', start + 1)
        functions.append(source[start:end].replace('control::', ''))
    body = '\n'.join(functions)
    codes = sorted(set(re.findall(r'MessageCode::(\w+)', body)))
    msgs = sorted(set(re.findall(r'Msg::(\w+)', body)))
    objects = sorted((set(re.findall(r'(\w+)->', body)) |
                      set(re.findall(r'(\w+)\s*[!=]=\s*caller', body))) - {'TableG', 'caller'})
    head = '''#include <cassert>
#include <cstdio>
#include <vector>
#include <utility>
'''
    head += 'enum class MessageCode {' + ','.join(codes) + '};\n'
    head += 'enum class Msg {' + ','.join(msgs) + '};\n'
    head += r'''
namespace pb { bool FullTiltMode; Msg last; const char* get_rc_string(Msg m) {last=m; return "message";} bool chk_highscore(){return false;} }
enum class MidiTracks {Track1};
namespace midi {void play_track(MidiTracks,bool){}}
struct TPinballComponent {
 int MessageField=0, BmpIndex=0, count=0; bool on=false;
 std::vector<std::pair<Msg,float>> displays;
 int Message(MessageCode c,float) {
  if(c==MessageCode::TLightResetAndTurnOn) on=true;
  if(c==MessageCode::TLightResetAndTurnOff || c==MessageCode::Reset) on=false;
  return count;
 }
 bool light_on(){return on;}
 void Display(const char*,float t){displays.push_back({pb::last,t});}
 void Play(void*,const char*){}
};
'''
    head += '\n'.join('TPinballComponent obj_' + n + ', *' + n + '=&obj_' + n + ';' for n in objects)
    head += r'''
struct Table {
 bool TiltLockFlag=false; int MultiballCount=0, UnknownP78=3, BonusScore=0, ExtraBalls=0, CurrentPlayer=0, PlayerCount=2, BallCount=3, switches=0;
 TPinballComponent lightGroup; TPinballComponent* LightGroup=&lightGroup;
 void ChangeBallCount(int n){BallCount=n;}
 void Message(MessageCode c,float){if(c==MessageCode::SwitchToNextPlayer){switches++; CurrentPlayer=(CurrentPlayer+1)%PlayerCount;} if(c==MessageCode::ClearTiltLock)TiltLockFlag=false;}
} table, *TableG=&table;
bool table_unlimited_balls=false;
Msg RankRcArray[10]{};
void MissionControl(MessageCode,TPinballComponent*){}
int SpecialAddScore(int n,bool=false){return n;}
bool AddRankProgress(int){return true;}
void AdvanceWormHoleDestination(int){}
void GravityWellKickoutControl(MessageCode,TPinballComponent*){}
void table_set_replay(float);
'''
    reset = '\nvoid reset(){ table=Table{};\n' + '\n'.join('*' + n + '=TPinballComponent{};' for n in objects)
    reset += '\nlite56->MessageField=1; lite107->MessageField=15; lite104->MessageField=15; bump1->BmpIndex=1; bump5->BmpIndex=1; lite20->on=true; middle_circle->count=2; }\n'
    tests = r'''
bool awarded(){for(auto d:info_text_box->displays) if(d.first==Msg::STRING101 && d.second==-1.0f)return true; return false;}
int main(){
for(bool ft:{false,true}) { pb::FullTiltMode=ft;
'''
    for name, caller in {**FINALS, **INTERMEDIATE}.items():
        expected = 'ft' if name in FINALS else 'false'
        f = name + 'Controller'
        tests += f'// {name}\n'
        for event in ['ControlMissionComplete', 'ControlMissionStarted']:
            tests += f'reset(); {f}(MessageCode::{event},nullptr); assert(!awarded() && !lite199->on);\n'
        event = 'TBumperSetBmpIndex' if name == 'AlienMenace' else 'ControlCollision'
        tests += f'reset(); {f}(MessageCode::{event},{caller}); assert(awarded()=={expected}); assert(lite199->on=={expected});\n'
        if name in FINALS:
            tests += 'assert(lite198->MessageField==1);\n'
        else:
            next_state = dict(zip(INTERMEDIATE, [20,21,22,23,24,1,25,26,27,28,29,30,31]))[name]
            tests += f'assert(lite198->MessageField=={next_state});\n'
    tests += r'''
// A nonfinal hit does not award; unrelated hits do not award.
reset(); lite56->MessageField=2; PracticeMissionController(MessageCode::ControlCollision,bump1); assert(!awarded());
reset(); PracticeMissionController(MessageCode::ControlCollision,ramp); assert(!awarded());
// A replay is boolean, consumed by a drain without ball loss/player rotation.
reset(); table_set_replay(-1); table_set_replay(-1); lite200->on=false;
BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(!lite199->on && lite200->on && table.BallCount==3 && table.switches==0 && table.UnknownP78==2);
// After shoot-again protection expires, the next drain loses a ball and rotates.
lite200->on=false; BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(table.BallCount==2 && table.switches==1 && table.CurrentPlayer==1 && !lite199->on);
// Tilt cancels the award before considering the replay branch.
reset(); table_set_replay(-1); table.TiltLockFlag=true; lite200->on=false;
BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(!lite199->on && table.BallCount==2 && table.switches==1);
// Ball-save protection takes precedence, preserving an earned replay.
reset(); table_set_replay(-1); lite200->on=true;
BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(lite199->on && table.BallCount==3 && table.switches==0);
}
puts("PASS: 16 finals, 13 intermediate/exception states, starts, wrong/nonfinal hits, replay drain/tilt/player routing in both modes");
}
'''
    return head + body + reset + tests


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='pinball-replays-') as tmp:
        cpp = Path(tmp) / 'mission_replays.cpp'
        exe = Path(tmp) / 'mission_replays.exe'
        cpp.write_text(generate())
        compiler = [args.cxx] + (['c++'] if Path(args.cxx).stem == 'zig' else [])
        subprocess.run(compiler + ['-std=c++11', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
