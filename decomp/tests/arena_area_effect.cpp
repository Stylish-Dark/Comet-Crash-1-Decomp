#include "comet/arena_area_effect.hpp"
#include <cassert>
#include <cmath>
#include <bit>
#include <cfenv>
#include <limits>
using namespace comet::decomp;
int main(){
 ArenaAreaEffectQuery q;q.center={5.51f,0,5.5f,0};q.cell={5,5};q.entry={0,17,40,40,200,50,100,20};q.radius=1;q.damage=100;q.impulse_strength=0;q.stage_mask=32;q.owner_mask=1;
 const auto hit=evaluate_arena_area_effect_cubic(q);assert(hit && hit->index==17);assert(hit->health_delta<-75 && hit->health_delta>-81);assert(hit->impulse[0]==0);
 q.stage_mask=16;assert(!evaluate_arena_area_effect_cubic(q));q.stage_mask=32;q.owner_mask=2;assert(!evaluate_arena_area_effect_cubic(q));q.owner_mask=1;
 q.entry[3]=2;q.exclude_marker_two=true;assert(!evaluate_arena_area_effect_cubic(q));q.exclude_marker_two=false;assert(evaluate_arena_area_effect_cubic(q));
 {auto shell=q;shell.entry[3]=40;shell.entry[4]=200;shell.center={5.495f,0,5.5f,0};shell.radius=.6f;const auto r=evaluate_arena_area_effect_cubic(shell);assert(r && r->health_delta>0);}
 {auto exact=q;exact.entry={0,17,57,40,21,36,90,104};exact.center={5.27625846862793f,.09392744302749634f,6.585141181945801f,0};exact.stage_mask=183;exact.owner_mask=206;exact.radius=1.0584908208067059f;exact.damage=7.485624625651644f;exact.impulse_strength=1.7656296034542385f;const auto r=evaluate_arena_area_effect_cubic(exact);assert(r && std::bit_cast<unsigned>(r->health_delta)==0xc0956aadu);assert(std::bit_cast<unsigned>(r->impulse[0])==0xc0959569u);}
 {const int previous=std::fegetround();assert(std::fesetround(FE_UPWARD)==0);assert(evaluate_arena_area_effect_cubic(q));assert(std::fegetround()==FE_UPWARD);assert(std::fesetround(previous)==0);}
 q.entry[4]=100;q.impulse_strength=2;const auto near=evaluate_arena_area_effect_cubic(q);assert(near && near->health_delta==-100.f && near->impulse[0]<0);
 assert(arena_area_effect_cell_overlaps(q.center,q.cell,1));assert(!arena_area_effect_cell_overlaps({9,0,9,0},q.cell,1));
 {auto bad=q;for(float v:{std::numeric_limits<float>::max(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::denorm_min()}){bad=q;bad.center[0]=v;bool rejected=false;try{evaluate_arena_area_effect_cubic(bad);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);}}
 {auto bad=q;for(float v:{std::numeric_limits<float>::min(),std::numeric_limits<float>::denorm_min(),0.f,-1.f,65.f}){bad.radius=v;bool rejected=false;try{evaluate_arena_area_effect_cubic(bad);}catch(const std::invalid_argument&){rejected=true;}assert(rejected);}}
 bool invalid=false;q.radius=0;try{evaluate_arena_area_effect_cubic(q);}catch(const std::invalid_argument&){invalid=true;}assert(invalid);
}
