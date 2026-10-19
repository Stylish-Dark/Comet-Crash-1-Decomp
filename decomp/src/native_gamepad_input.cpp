#include "comet/native_gamepad_input.hpp"
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
float decode_arena_pad_axis(std::uint8_t raw){
 // PPU F1728..F1868: independent axis center/scale, strict 20% deadzone,
 // signed offset, then 1.25 rescale. Positive endpoint deliberately stays <1.
 const int previous=std::fegetround();
 if(previous<0 || std::fesetround(FE_TONEAREST)!=0)throw std::runtime_error("pad rounding unavailable");
 volatile float centered=float(int(raw)-128)*.0078125f;
 float result=0;
 if(std::fabs(float(centered))>=.2f){
  volatile float shifted=float(centered)+(centered<0?.2f:-.2f);
  volatile float scaled=float(shifted)*1.25f;result=scaled;
 }
 std::fesetround(previous);return result;
}
NativeGamepadActions NativeGamepadInput::advance(NativeGamepadSample s,float elapsed,bool enabled){
 if(!std::isfinite(elapsed)||elapsed<0||elapsed>.25f)throw std::invalid_argument("invalid gamepad elapsed time");
 NativeGamepadActions a;
 if(!enabled || !s.connected){cancel();return a;}
 auto axis=[](std::int16_t value){
  const auto raw=static_cast<std::uint8_t>(((int(value)+32768)*255+32767)/65535);
  return decode_arena_pad_axis(raw);
 };
 const float mx=axis(s.x),mz=axis(s.z);
 int dx=mx<0?-1:mx>0?1:0,dz=mz<0?-1:mz>0?1:0;
 if(s.buttons&(PadLeft|PadRight))dx=int(bool(s.buttons&PadRight))-int(bool(s.buttons&PadLeft));
 if(s.buttons&(PadUp|PadDown))dz=int(bool(s.buttons&PadDown))-int(bool(s.buttons&PadUp));
 if(!armed_){if(!s.buttons && !dx && !dz)armed_=true;return a;}
 a.move_x=mx;a.move_z=mz;
 const auto pressed=s.buttons&~previous_;previous_=s.buttons;
 a.build=pressed&PadBuild;a.cancel=pressed&PadCancel;a.factory=pressed&PadFactory;
 a.next_kind=pressed&PadNextKind;a.next_owner=pressed&PadNextOwner;
 a.pause=pressed&PadPause;a.restart=pressed&PadRestart;
 a.build_delta=int(bool(pressed&PadNextBuild))-int(bool(pressed&PadPreviousBuild));
 a.release_held=s.buttons&PadRelease;
 if(dx!=dx_ || dz!=dz_){dx_=dx;dz_=dz;repeat_=.2f;a.dx=dx;a.dz=dz;}
 else if(dx || dz){
  repeat_-=elapsed;
  if(repeat_<0){a.dx=dx;a.dz=dz;repeat_=.08f;}
 }
 return a;
}
}
