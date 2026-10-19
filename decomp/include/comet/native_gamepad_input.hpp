#pragma once
#include <cstdint>
namespace comet::decomp {
float decode_arena_pad_axis(std::uint8_t raw);
enum NativePadButton : std::uint32_t {
 PadBuild=1u<<0,PadCancel=1u<<1,PadRelease=1u<<2,PadFactory=1u<<3,
 PadPreviousBuild=1u<<4,PadNextBuild=1u<<5,PadNextKind=1u<<6,
 PadNextOwner=1u<<7,PadPause=1u<<8,PadRestart=1u<<9,
 PadUp=1u<<10,PadDown=1u<<11,PadLeft=1u<<12,PadRight=1u<<13
};
struct NativeGamepadSample {bool connected=false;std::int16_t x=0,z=0;std::uint32_t buttons=0;};
struct NativeGamepadActions {
 int dx=0,dz=0,build_delta=0;
 float move_x=0,move_z=0;
 bool build=false,cancel=false,release_held=false,factory=false,next_kind=false,
      next_owner=false,pause=false,restart=false;
};
// Preview grid/actions adapter; original craft movement is a separate boundary.
class NativeGamepadInput {
 bool armed_=false;
 std::uint32_t previous_=0;
 int dx_=0,dz_=0;
 float repeat_=0;
public:
 void cancel(){armed_=false;previous_=0;dx_=dz_=0;repeat_=0;}
 NativeGamepadActions advance(NativeGamepadSample,float elapsed,bool enabled);
};
}
