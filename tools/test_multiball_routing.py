"""Test production lock/drain controllers with observable component doubles.

Reuses the mission harness; does not test actual sink timers or ball creation.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_mission_replays import generate as mission_harness
from test_ball_collision import function


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('Mission harness changed; update the explicit adapter: ' + old)
    return text.replace(old, new)


def generate():
    source = mission_harness()
    source = replace_once(source, 'enum class MidiTracks {Track1};',
                          'enum class MidiTracks {Track1,Track3};')
    source = replace_once(source, 'enum class MessageCode {',
                          'enum class MessageCode {PlungerRelaunchBall,')
    source = replace_once(source, 'enum class Msg {', 'enum class Msg {STRING117,STRING102,')
    source = replace_once(source, 'bool TiltLockFlag=false;', 'int BallLockedCounter=0; bool TiltLockFlag=false;')
    source = replace_once(source, 'TPinballComponent lightGroup;',
                          'TPinballComponent plungerObject; TPinballComponent* Plunger=&plungerObject; TPinballComponent lightGroup;')
    source = replace_once(source, 'int Message(MessageCode c,float) {',
                          'std::vector<std::pair<MessageCode,float>> messages; int Message(MessageCode c,float value) { messages.push_back({c,value});')
    # These sound objects are not used by the existing mission harness.
    additions = 'TPinballComponent sound41, sound44, *soundwave41=&sound41, *soundwave44=&sound44;\n'
    additions += function('control.cpp', 'void control::table_set_multiball(').replace('control::', '') + '\n'
    additions += function('control.cpp', 'void control::table_bump_ball_sink_lock(').replace('control::', '') + '\n'
    source = replace_once(source, 'int main(){', additions + r'''
int main(){
reset(); table.MultiballCount=1;
table_bump_ball_sink_lock();
assert(table.MultiballCount==0 && table.BallLockedCounter==1);
assert(table.Plunger->messages.back()==std::make_pair(MessageCode::PlungerRelaunchBall,2.0f));
table.MultiballCount=1; // Replacement feed is external to this harness.
table_bump_ball_sink_lock();
assert(table.MultiballCount==0 && table.BallLockedCounter==2);
table.MultiballCount=1;
table_bump_ball_sink_lock();
assert(table.MultiballCount==3 && table.BallLockedCounter==0);
for(auto sink:{sink1,sink2,sink3})
 assert(sink->messages.back()==std::make_pair(MessageCode::TSinkResetTimer,2.0f));
for(auto lamp:{lite38,lite39,lite40})
 assert(lamp->messages.back()==std::make_pair(MessageCode::TLightFlasherStartTimed,-1.0f));
auto calls=sink1->messages.size();
table_set_multiball(2); table_bump_ball_sink_lock();
assert(table.MultiballCount==3 && table.BallLockedCounter==0 && sink1->messages.size()==calls);
// TDrain decrements the active counter before it calls BallDrainControl.
lite199->on=false; lite200->on=false;
table.MultiballCount=2; BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(table.BallCount==3 && table.switches==0);
assert(lite40->messages.back().first==MessageCode::TLightResetAndTurnOff);
table.MultiballCount=1; BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(table.BallCount==3 && table.switches==0);
assert(lite38->messages.back().first==MessageCode::TLightResetAndTurnOff);
assert(lite39->messages.back().first==MessageCode::TLightResetAndTurnOff);
table.MultiballCount=0; BallDrainControl(MessageCode::ControlCollision,nullptr);
assert(table.BallCount==2 && table.switches==1);
puts("PASS: production lock 1/2/3, sink scheduling, reentry guard, drain 3/2/1 routing");
''')
    return source


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='pinball-multiball-') as tmp:
        cpp, exe = Path(tmp)/'multiball.cpp', Path(tmp)/'multiball.exe'
        cpp.write_text(generate())
        compiler = [args.cxx] + (['c++'] if Path(args.cxx).stem == 'zig' else [])
        subprocess.run(compiler + ['-std=c++11', str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
