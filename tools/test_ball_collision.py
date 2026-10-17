"""Compile unmodified production collision/math methods in a minimal state harness.

No copied collision algorithm: methods are extracted with balanced braces. The
ball state replaces construction/rendering only. This is not a whole-game test.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(file, signature):
    text = (ROOT / 'SpaceCadetPinball' / file).read_text()
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def generate():
    source = r'''
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <vector>
#include "maths.h"
struct TEdgeSegment {};
class TBall : public TEdgeSegment {
public:
 vector3 Position{}, Direction{};
 float Speed=0, Radius=1;
 bool CollisionDisabledFlag=false, ActiveFlag=true;
 int CollisionMask=1;
 void EdgeCollision(TBall*,float);
 float FindCollisionDistance(const ray_type&);
};
struct Table {std::vector<TBall*> BallList;} table;
namespace pb {
 Table* MainTable=&table;
 float BallToBallCollisionDistance=100;
 float BallToBallCollision(const ray_type&, const TBall&, TEdgeSegment**, float);
}
'''
    for file, signature in [
        ('maths.cpp', 'float maths::normalize_2d('),
        ('maths.cpp', 'float maths::DotProduct('),
        ('maths.cpp', 'vector2 maths::vector_sub('),
        ('maths.cpp', 'float maths::ray_intersect_circle('),
        ('TBall.cpp', 'void TBall::EdgeCollision('),
        ('TBall.cpp', 'float TBall::FindCollisionDistance('),
        ('pb.cpp', 'float pb::BallToBallCollision('),
    ]:
        source += function(file, signature) + '\n'
    return source + r'''
bool near(float a,float b){return std::abs(a-b)<0.0002f;}
vector2 velocity(const TBall& b){return {b.Direction.X*b.Speed,b.Direction.Y*b.Speed};}
void collide(float ax,float ay,float bx,float by){
 TBall a,b; a.Position={-2,0}; b.Position={0,0};
 a.Direction={ax,ay}; a.Speed=maths::normalize_2d(a.Direction);
 b.Direction={bx,by}; b.Speed=maths::normalize_2d(b.Direction);
 b.EdgeCollision(&a,0);
 auto av=velocity(a), bv=velocity(b);
 // Equal masses exchange normal velocity and retain tangential velocity.
 assert(near(av.X,bx)&&near(av.Y,ay)&&near(bv.X,ax)&&near(bv.Y,by));
 assert(near(av.X+bv.X,ax+bx)&&near(av.Y+bv.Y,ay+by));
 assert(near(av.X*av.X+av.Y*av.Y+bv.X*bv.X+bv.Y*bv.Y,ax*ax+ay*ay+bx*bx+by*by));
 assert(a.CollisionDisabledFlag);
}
int main(){
 collide(4,0,0,0); collide(4,0,-3,0); collide(4,2,-1,-3); collide(0,0,0,0);
 TBall a,b,c; a.Position={-5,0}; b.Position={0,0}; c.Position={3,0};
 a.Direction={1,0}; a.Speed=4;
 ray_type ray{{-5,0},{1,0},10,0.002f,1};
 assert(near(b.FindCollisionDistance(ray),3));
 table.BallList={&a,&c,&b}; TEdgeSegment* edge=nullptr;
 assert(near(pb::BallToBallCollision(ray,a,&edge,100),2.998f)&&edge==&b);
 // Inactive balls, self, and disjoint collision layers are excluded.
 b.ActiveFlag=false; c.CollisionMask=2; edge=nullptr;
 assert(pb::BallToBallCollision(ray,a,&edge,100)==100&&edge==nullptr);
 b.ActiveFlag=true; edge=&c;
 assert(pb::BallToBallCollision(ray,a,&edge,2)==2&&edge==&c);
 ray.Direction={-1,0}; assert(b.FindCollisionDistance(ray)>=1e9f);
 ray.Direction={1,0}; ray.Origin.Y=3; assert(b.FindCollisionDistance(ray)>=1e9f);
 // Travel to contact is applied before computing the collision normal.
 a.Position={-5,0}; a.Direction={1,0}; a.Speed=4;
 b.Position={0,0}; b.Direction={0,0}; b.Speed=0;
 b.EdgeCollision(&a,3); assert(near(a.Position.X,-2)&&near(a.Speed,0)&&near(b.Speed,4));
 puts("PASS: production ball collision response, geometry, nearest edge, active/mask/self filters, wall precedence");
}
'''


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='pinball-collision-') as tmp:
        cpp, exe = Path(tmp)/'collision.cpp', Path(tmp)/'collision.exe'
        cpp.write_text(generate())
        compiler = [args.cxx] + (['c++'] if Path(args.cxx).stem == 'zig' else [])
        subprocess.run(compiler + ['-std=c++11', '-I', str(ROOT/'SpaceCadetPinball'), str(cpp), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
