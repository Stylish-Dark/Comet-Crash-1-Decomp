#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
namespace comet::decomp::detail {
// Numeric estimate buckets used by the recovered distance sequence. This is
// native arithmetic data, not SPU execution or generated instruction code.
constexpr std::array<std::uint32_t,64> inverse_root_buckets{
 0x350160,0x34E954,0x2F993D,0x2F993D,0x2AA523,0x2AA523,0x26190D,0x26190D,
 0x21E4F9,0x21E4F9,0x1E00E9,0x1E00E9,0x1A5CD9,0x1A5CD9,0x16F8CB,0x16F8CB,
 0x13CCC0,0x13CCC0,0x10CCB3,0x10CCB3,0x0E00AA,0x0E00AA,0x0B58A1,0x0B58A1,
 0x08D498,0x08D498,0x067491,0x067491,0x043089,0x043089,0x020C83,0x020C83,
 0x7FFDF4,0x7FD1DE,0x7859C8,0x783DBA,0x71559C,0x71559C,0x6AE57C,0x6AE57C,
 0x64F561,0x64F561,0x5F7149,0x5F7149,0x5A4D33,0x5A4D33,0x55811F,0x55811F,
 0x51050F,0x51050F,0x4CC8FE,0x4CC8FE,0x48D0F0,0x48D0F0,0x4510E4,0x4510E4,
 0x4180D7,0x4180D7,0x3E24CC,0x3E24CC,0x3AF4C3,0x3AF4C3,0x37E8BA,0x37E8BA};

// FRSQEST/FI finite-normal estimate. Caller owns round-toward-zero scope.
inline float inverse_root_estimate(float squared) {
 const auto bits=std::bit_cast<std::uint32_t>(squared),exponent=(bits>>23)&255u;
 const auto estimate=((190u-(exponent+1u)/2u)<<23)|inverse_root_buckets[(bits>>18)&63u];
 const float base=std::bit_cast<float>(estimate&0xfffffc00u);
 const float step=std::ldexp(float(estimate&1023u),int((estimate>>23)&255u)-127-13);
 const float fraction=std::ldexp(float(bits&0x7ffffu),-19);
 volatile float product=step*fraction;
 volatile float result=base-product;
 return result;
}
} // namespace comet::decomp::detail
