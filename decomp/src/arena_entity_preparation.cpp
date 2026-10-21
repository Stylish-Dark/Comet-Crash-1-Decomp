#include "comet/arena_entity_preparation.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
using V=std::array<float,4>;
using U=std::array<std::uint32_t,4>;
std::uint32_t word(const auto& b,unsigned o){
 return std::uint32_t(b[o])<<24|std::uint32_t(b[o+1])<<16|std::uint32_t(b[o+2])<<8|b[o+3];
}
void put(ArenaEntityWorkContext& b,unsigned o,std::uint32_t v){
 for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));
}
void half(ArenaEntityWorkContext& b,unsigned o,unsigned v){b[o]=std::uint8_t(v>>8);b[o+1]=std::uint8_t(v);}
float scalar(const auto& b,unsigned o){return std::bit_cast<float>(word(b,o));}
void fp(ArenaEntityWorkContext& b,unsigned o,float f){put(b,o,std::bit_cast<std::uint32_t>(f));}
// Source packed byte/halfword extraction rotates an entire quadword. Retain
// the secondary lanes too: subsequent context writes store all four words.
U byte_lanes(const auto& b,unsigned o){
 U v{};for(unsigned i=0;i<4;++i)v[i]=b[(o&~15u)|((o+4*i)&15u)];return v;
}
U half_lanes(const auto& b,unsigned o){
 U v{};for(unsigned i=0;i<4;++i){unsigned p=(o&~15u)|((o+4*i)&15u);v[i]=unsigned(b[p])<<8|b[p+1];}return v;
}
void words(ArenaEntityWorkContext& b,unsigned o,const U& v){for(unsigned i=0;i<4;++i)put(b,o+4*i,v[i]);}
struct UnsupportedArithmetic {};
float checked(float f){
 if(!std::isfinite(f)||std::fpclassify(f)==FP_SUBNORMAL)throw UnsupportedArithmetic{};
 return f;
}
float mul(float a,float b){volatile float v=a*b;return checked(v);}
float add(float a,float b){volatile float v=a+b;return checked(v);}
float ma(float a,float b,float c){return checked(std::fma(a,b,c));}
float nm(float a,float b,float c){return checked(std::fma(a,-b,c));}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL&&std::abs(f)<=1000000;}
struct Rounding{
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("entity staging rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
V rotate_forward(const V& q){
 // 9B68..9D38: original ordered quaternion rotation of (0,0,-1,0).
 // Keep the fused operations and all four lanes, including signed zero.
 const V axis{0,0,-1,0},up{0,-1,0,0},left{-1,0,0,0};
 V axis_product{},first{},intermediate{},result{};
 for(unsigned i=0;i<4;++i)axis_product[i]=mul(q[i],axis[i]);
 for(unsigned i=0;i<4;++i)first[i]=add(ma(up[i],q[(i+1)%4],axis_product[i]),axis_product[(i+2)%4]);
 const std::array<unsigned,4> yzx{1,2,0,3},zxy{2,0,1,3};
 for(unsigned i=0;i<4;++i)intermediate[i]=nm(q[zxy[i]],up[i],ma(q[yzx[i]],left[i],mul(q[3],axis[i])));
 V weighted{};
 for(unsigned i=0;i<4;++i)weighted[i]=ma(q[3],intermediate[i],mul(first[0],q[i]));
 for(unsigned i=0;i<4;++i)result[i]=nm(q[zxy[i]],intermediate[yzx[i]],ma(q[yzx[i]],intermediate[zxy[i]],weighted[i]));
 return result;
}
}
bool stage_arena_entity_preparation(ArenaEntityWorkContext& context,
 std::span<const std::uint8_t,0x2900> snapshot){
 const unsigned width=word(snapshot,0x28d8),depth=word(snapshot,0x28dc);
 if(!width||width>24||!depth||depth>24||(context[0x33]>>5)>=4)return false;
 V q{};for(unsigned i=0;i<4;++i){q[i]=scalar(context,0x20+4*i);if(!number(q[i]))return false;}
 for(unsigned i=0;i<3;++i)if(!number(scalar(context,4*i)))return false;
 Rounding rounding;auto next=context;
 std::copy_n(context.begin(),16,next.begin()+0x480);
 std::copy_n(context.begin()+0x40,16,next.begin()+0x4a0);
 std::copy_n(context.begin(),16,next.begin()+0x490);put(next,0x494,0);
 put(next,0x5a0,1);
 for(unsigned o:{0x590,0x591,0x592,0x593,0x594,0x596,0x598,0x599,0x59a})next[o]=0;
 V forward;try{forward=rotate_forward(q);}catch(const UnsupportedArithmetic&){return false;}
 for(unsigned i=0;i<4;++i){if(!std::isfinite(forward[i]))return false;fp(next,0x4b0+4*i,forward[i]);}
 const float percent=std::bit_cast<float>(0x3c23d70au);
 fp(next,0x4e0,mul(float(context[0x0e]),percent));
 fp(next,0x4e4,mul(float(context[0x32]),percent));
 put(next,0x4e8,word(context,0x38));
 auto owners=byte_lanes(context,0x33),stages=owners,stage_masks=owners;
 for(unsigned i=0;i<4;++i){owners[i]>>=5;stages[i]=(stages[i]>>2)&7u;stage_masks[i]=1u<<stages[i];}
 half(next,0x542,owners[0]);half(next,0x544,owners[0]);
 words(next,0x510,stages);words(next,0x520,stage_masks);
 words(next,0x4f0,half_lanes(context,0x0c));
 words(next,0x500,byte_lanes(context,0x0f));
 fp(next,0x570,100);
 std::copy_n(snapshot.begin()+0x2710,16,next.begin()+0x4c0);
 std::fill_n(next.begin()+0x4d0,16,0);
 const auto visibility=half_lanes(snapshot,0x2720+2*owners[0]);words(next,0x530,visibility);
 // 9CE8/9DB0 truncate toward zero, then clamp to [0,extent-1].
 const int x=std::clamp(int(scalar(context,0)),0,int(width)-1);
 const int z=std::clamp(int(scalar(context,8)),0,int(depth)-1);
 half(next,0x552,unsigned(x));half(next,0x562,unsigned(z));
 const unsigned shift=visibility[0]&63u; // ROTMA's six-bit count
 const auto mask=snapshot[16*(24*unsigned(z)+unsigned(x))+3];
 next[0x595]=!(word(context,0x60)&0x00080000u)&&shift<8?std::uint8_t((mask>>shift)&1u):0;
 context=next;return true; // next source operation is1AD8, not the controller
}
}
