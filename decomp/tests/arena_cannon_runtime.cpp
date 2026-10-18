#include "../src/arena_cannon_runtime.hpp"
#include "comet/arena_entity_core.hpp"
#include <bit>
#include <cassert>
using namespace comet::decomp;
namespace {
void word(ArenaEntitySlot&s,unsigned o,unsigned v){for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
ArenaEntitySlot entity(unsigned kind,std::array<float,4> position){
 ArenaEntitySlot slot{};ArenaEntityCoreParameters p;
 p.type=static_cast<std::uint8_t>(kind);p.model_slot=kind==3?27:1;p.owner=0;
 p.stage=kind==3?1:5;p.quantity=6;p.flags=0x6000;p.position=position;
 p.scalar_58=1;p.scalar_38=p.scalar_64=170;
 initialize_arena_entity_core(std::span<std::uint8_t,128>(slot.data(),128),p);
 return slot;
}
ArenaEntityStorage world(unsigned capacity){
 ArenaEntityStorage w;for(auto &bank:w.banks)bank.resize(capacity);
 reset_arena_entity_index_pool(w,capacity);return w;
}
}
int main(){
 auto w=world(128);ArenaRoutingGrid grid;ArenaRandom random(3);
 std::array<ArenaRoutePlayer,1> players{{{{0,0},0,false}}};
 std::vector<std::array<float,4>> pending(128);
 for(unsigned n=0;n<126;n++)assert(insert_arena_entity(w,{2,1},entity(41,{2.5f,0,1.5f,0})));
 auto shot=entity(3,{.9f,0,1.5f,0});word(shot,0x40,std::bit_cast<unsigned>(30.f));
 auto id=insert_arena_entity(w,{0,1},shot);assert(id);
 auto tick=stage_arena_cannons(w,grid,players,.05f,random,pending);
 assert(tick.retired.size()==1 && tick.retired[0]==*id);
 assert(w.banks[0][*id][15]==0 && w.banks[0]==w.banks[1]);
 assert(w.summary_counts[1*24+2]==126 && w.summary_counts[1*24]==0);
 assert(w.free_indices.back()==*id);
 // Preview age cap retires a missed round and does not consume target RNG.
 w=world(1);pending.resize(1);shot=entity(3,{1.5f,0,1.5f,0});word(shot,0x80,600);
 assert(insert_arena_entity(w,{1,1},shot));auto before=random;
 tick=stage_arena_cannons(w,grid,players,.05f,random,pending);
 assert(tick.retired.size()==1 && w.free_indices.size()==1);
 assert(random.next()==before.next());
 // Constructor-style nonmovable shots must retire before escaping storage's Y limit.
 w=world(1);shot=entity(3,{1.5f,.39f,1.5f,0});word(shot,0x60,2);
 word(shot,0x44,std::bit_cast<unsigned>(-30.f));assert(insert_arena_entity(w,{1,1},shot));
 bool retired=false;
 for(unsigned n=0;n<100 && !retired;n++)retired=!stage_arena_cannons(w,grid,players,.05f,random,pending).retired.empty();
 assert(retired && w.free_indices.size()==1);
 // A legal high velocity can cross the native +/-64 storage domain in one step.
 w=world(1);shot=entity(3,{23.5f,0,1.5f,0});word(shot,0x60,2);
 word(shot,0x40,std::bit_cast<unsigned>(1024.f));assert(insert_arena_entity(w,{23,1},shot));
 tick=stage_arena_cannons(w,grid,players,.05f,random,pending);
 assert(tick.retired.size()==1 && w.free_indices.size()==1);

}
