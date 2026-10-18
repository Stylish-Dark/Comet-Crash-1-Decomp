#include "comet/native_gamepad_input.hpp"
#include <cassert>
#include <bit>
#include <cfenv>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
int main(){
 assert(std::bit_cast<unsigned>(decode_arena_pad_axis(0))==0xbf800000u);
 assert(std::bit_cast<unsigned>(decode_arena_pad_axis(102))==0xbb7ffff0u);
 assert(decode_arena_pad_axis(103)==0 && decode_arena_pad_axis(153)==0);
 assert(std::bit_cast<unsigned>(decode_arena_pad_axis(154))==0x3b7ffff0u);
 assert(std::bit_cast<unsigned>(decode_arena_pad_axis(255))==0x3f7d8000u);
 std::uint64_t hash=14695981039346656037ull;
 for(unsigned i=0;i<256;++i)hash=(hash^std::bit_cast<std::uint32_t>(decode_arena_pad_axis(i)))*1099511628211ull;
 assert(hash==0x5c60b3ebccbac445ull); // all 256 untouched-original scalar instruction fixtures
 auto rounding=std::fegetround();std::fesetround(FE_UPWARD);
 assert(std::bit_cast<unsigned>(decode_arena_pad_axis(154))==0x3b7ffff0u && std::fegetround()==FE_UPWARD);std::fesetround(rounding);
 NativeGamepadInput input;NativeGamepadSample s;s.connected=true;
 assert(input.advance(s,.01f,true).dx==0);
 s.x=32767;auto a=input.advance(s,.01f,true);assert(a.dx==1 && a.dz==0);
 assert(input.advance(s,.1f,true).dx==0);
 assert(input.advance(s,.11f,true).dx==1);
 s.x=0;s.z=-32768;a=input.advance(s,.01f,true);assert(a.dx==0 && a.dz==-1);
 s.x=s.z=0;input.advance(s,.01f,true);
 s.buttons=PadBuild;a=input.advance(s,.01f,true);assert(a.build);
 assert(!input.advance(s,.01f,true).build);
 s.buttons=PadRelease;a=input.advance(s,.01f,true);assert(a.release_held);
 input.cancel();a=input.advance(s,.01f,true);assert(!a.release_held);
 s.buttons=0;input.advance(s,.01f,true);s.buttons=PadRelease;
 assert(input.advance(s,.01f,true).release_held);
 a=input.advance(s,.01f,false);assert(!a.release_held);
 assert(!input.advance(s,.01f,true).release_held); // regain requires neutral
 s.buttons=0;input.advance(s,.01f,true);s.buttons=PadPause|PadNextOwner;
 a=input.advance(s,.01f,true);assert(a.pause && a.next_owner);
 s.connected=false;assert(!input.advance(s,.01f,true).pause);
 s.connected=true;s.buttons=PadBuild;assert(!input.advance(s,.01f,true).build);
 s.buttons=0;input.advance(s,.01f,true);s.buttons=PadBuild;
 s.buttons=PadFactory|PadNextKind|PadNextBuild|PadRestart|PadCancel|PadLeft|PadRight|PadUp;
 a=input.advance(s,.01f,true);
 assert(a.factory && a.next_kind && a.build_delta==1 && a.restart && a.cancel && a.dx==0 && a.dz==-1);
 s.buttons=PadPreviousBuild;assert(input.advance(s,.01f,true).build_delta==-1);
 s.buttons=0;input.advance(s,.01f,true);s.buttons=PadBuild;
 bool rejected=false;try{input.advance(s,std::numeric_limits<float>::quiet_NaN(),true);}catch(const std::invalid_argument&){rejected=true;}
 assert(rejected && input.advance(s,.01f,true).build);
}
