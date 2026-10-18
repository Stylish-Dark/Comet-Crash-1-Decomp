#include "comet/native_deployment_input.hpp"
#include "comet/native_mission_outcome.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace comet::decomp {
namespace {constexpr std::array<std::uint8_t,7> ground_kinds{40,41,43,44,45,46,47};}
NativeDeploymentInput::NativeDeploymentInput(NativeConstructionScene &scene):scene_(scene){
 const auto &controls=scene.simulation().ground_deployment();
 for(std::size_t i=0;i<scene.simulation().accounts().size();++i)if(controls[i])kinds_[i]=controls[i]->kind;
 for(std::size_t i=0;i<scene.simulation().accounts().size();++i){owner_=i;if(controllable())return;}
 owner_=0;
}
bool NativeDeploymentInput::controllable() const {
 const auto &sim=scene_.simulation();
 return !sim.native_opponents()[owner_] && !sim.accounts()[owner_].route.inactive;
}
NativeDeploymentInput::~NativeDeploymentInput(){release();}
void NativeDeploymentInput::cycle_kind(int direction){
 if(direction==0 || !controllable())return;
 const auto found=std::find(ground_kinds.begin(),ground_kinds.end(),kind());
 const auto index=static_cast<std::size_t>(found-ground_kinds.begin());
 kinds_[owner_]=ground_kinds[(index+ground_kinds.size()+(direction>0?1:-1))%ground_kinds.size()];
 if(held_)scene_.set_ground_deployment(owner_,{kind(),1});
}
void NativeDeploymentInput::next_owner(){
 release();
 const auto count=scene_.simulation().accounts().size();
 for(std::size_t i=0;i<count;++i){owner_=(owner_+1)%count;if(controllable())return;}
}
void NativeDeploymentInput::press(){if(controllable() && scene_.set_ground_deployment(owner_,{kind(),1}))held_=true;}
void NativeDeploymentInput::release(){if(held_){scene_.set_ground_deployment(owner_,{kind(),0});held_=false;}}
std::array<std::string,5> NativeDeploymentInput::status_lines(bool playing) const {
 const auto &sim=scene_.simulation();const auto &quota=sim.unit_quotas()[owner_];
 const auto &control=sim.ground_deployment()[owner_];
 const bool releasing=control && control->strength>0;
 std::ostringstream funds;funds<<std::fixed<<std::setprecision(1)<<sim.accounts()[owner_].resource;
 const auto outcome=evaluate_native_mission(sim,owner_);
 const char* state=outcome==NativeMissionOutcome::Victory?"VICTORY":outcome==NativeMissionOutcome::Defeat?"DEFEAT":outcome==NativeMissionOutcome::Draw?"DRAW":outcome==NativeMissionOutcome::InProgress?"ACTIVE":"UNCONFIGURED";
 std::string bases=std::string("MISSION ")+state+"  BASE COUNTERS";
 for(std::size_t i=0;i<sim.accounts().size();++i){const auto counter=sim.base_counter(i);bases+="  "+std::to_string(i+1)+":"+(counter?std::to_string(*counter):"--");}
 return {(controllable()?"PLAYER ":"OBSERVER ")+std::to_string(owner_+1)+"  FUNDS "+funds.str()+"  ACTIVE "+std::to_string(quota.active)+"  PENDING "+std::to_string(quota.pending),
 "RELEASE TYPE "+std::to_string(kind())+"  ORDERS "+std::to_string(quota.by_type[kind()-40])+"  "+(held_?"HELD":releasing?"SCRIPTED":"IDLE")+"  "+(playing?"PLAYING":"PAUSED"),
 bases,"Q/E TYPE  HOLD D RELEASE  TAB PLAYER  P PLAY  T TICK  F2 RESTART",
 "B BUILD  F5-F9 FACTORY  M MOVE  N DYNAMICS  C CONTACT"};
}
}
