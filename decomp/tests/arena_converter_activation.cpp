#include "comet/arena_converter_activation.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <limits>
using namespace comet::decomp;
namespace {
void put(ArenaEntitySlot&s,unsigned o,float f){unsigned v=std::bit_cast<unsigned>(f);for(unsigned i=0;i<4;i++)s[o+i]=v>>(24-8*i);}
float scalar(const ArenaEntitySlot&s,unsigned o){unsigned v=0;for(unsigned i=0;i<4;i++)v=(v<<8)|s[o+i];return std::bit_cast<float>(v);}
ArenaEntitySlot unit(float phase){ArenaEntitySlot s;s.fill(0xa5);s[15]=45;s[0x33]=20;put(s,0x98,phase);return s;}
}
int main(){
 auto s=unit(0),before=s;auto result=advance_arena_converter_activation(s,0);
 assert(result && *result==std::bit_cast<float>(0x3f333333u));assert(s[0xbb]==0);
 for(unsigned i=0;i<256;i++)if(i!=0xbb && (i<0x98||i>=0x9c))assert(s[i]==before[i]);
 s=unit(0);result=advance_arena_converter_activation(s,.1f);assert(result && s[0xbb]==1 && scalar(s,0x98)>0 && *result>.7f);
 s=unit(3.1415926f);result=advance_arena_converter_activation(s,.1f);assert(result && s[0xbb]==0 && *result==std::bit_cast<float>(0x3f333333u));
 s=unit(std::bit_cast<float>(0x40c90fdau));result=advance_arena_converter_activation(s,0);assert(result && scalar(s,0x98)==std::bit_cast<float>(0x40c90fdau));
 s=unit(6.2f);result=advance_arena_converter_activation(s,.1f);assert(result && scalar(s,0x98)<.2f);
 s=unit(.1001674f);assert(advance_arena_converter_activation(s,0)&&s[0xbb]==0);
 s=unit(.1001675f);assert(advance_arena_converter_activation(s,0)&&s[0xbb]==1);
 for(float elapsed:{-1.f,3.01f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){s=unit(0);before=s;assert(!advance_arena_converter_activation(s,elapsed));assert(s==before);}
 for(float cooldown:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){s=unit(0);put(s,0x8c,cooldown);before=s;assert(!advance_arena_converter_activation(s,.1f));assert(s==before);}
 s=unit(0);put(s,0x8c,1);put(s,0x18,std::numeric_limits<float>::quiet_NaN());before=s;assert(!advance_arena_converter_activation(s,.1f));assert(s==before);
 for(float phase:{-1.f,6.3f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){s=unit(phase);before=s;assert(!advance_arena_converter_activation(s,.1f));assert(s==before);}
 for(unsigned kind:{40u,44u,46u}){s=unit(0);s[15]=kind;before=s;assert(!advance_arena_converter_activation(s,.1f));assert(s==before);}
 s=unit(0);s[0x33]=24;before=s;assert(!advance_arena_converter_activation(s,.1f));assert(s==before);
 s=unit(1);put(s,0x8c,.05f);put(s,0x18,1);s[0xbb]=1;result=advance_arena_converter_activation(s,.1f);assert(result && *result==std::bit_cast<float>(0x3f199999u));assert(s[0xbb]==0 && scalar(s,0x98)==1 && scalar(s,0x8c)<0);
 result=advance_arena_converter_activation(s,.1f);assert(result && s[0xbb]==1 && scalar(s,0x98)>1);
 s=unit(1);put(s,0x8c,0);result=advance_arena_converter_activation(s,.1f);assert(result && scalar(s,0x98)>1);
 int rounding=fegetround();fesetround(FE_UPWARD);s=unit(0);assert(advance_arena_converter_activation(s,3));assert(fegetround()==FE_UPWARD);fesetround(rounding);
}
