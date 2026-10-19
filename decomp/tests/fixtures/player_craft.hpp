#pragma once
#include <array>
#include <cstdint>
// Synthetic inputs and FNV-1a fingerprints of all 256 original output bytes.
struct PlayerCraftFixture {std::uint32_t seed,owner;std::array<std::uint32_t,4> position;std::uint64_t output_hash;};
inline constexpr std::array<PlayerCraftFixture,16> player_craft_fixtures{{
 {2537769272u,0u,{{0x43e1f9afu,0x00000000u,0x441a481cu,0x3f2f8d47u}},0xa9922889212221a0ull},
 {3128916551u,1u,{{0x437a1fa9u,0x00000000u,0xc406e4d4u,0xbfd47d57u}},0x196f7864590f7798ull},
 {821851568u,2u,{{0x43c5999au,0x00000000u,0xc320dce7u,0xc11ab8cbu}},0x564240b9055d0e5bull},
 {2068244766u,3u,{{0xc3337434u,0x00000000u,0xc44087abu,0x3ed264dcu}},0xd6590ae028dd8d24ull},
 {2284733025u,0u,{{0xc423ab73u,0x00000000u,0xc431f012u,0x4108aca0u}},0x87300c2cdd23ef04ull},
 {2703198776u,1u,{{0x43d43c9du,0x00000000u,0xc41645d3u,0xc0eed9c7u}},0xa9cb563d2378a32eull},
 {1647948710u,2u,{{0x44600f05u,0x00000000u,0xc414cf7cu,0xc11d87f0u}},0xa3137db91aec55a6ull},
 {3881351532u,3u,{{0x4470e0adu,0x00000000u,0xc0fcae52u,0xbf100626u}},0xc535b1e65861472aull},
 {1060543951u,0u,{{0xc3ae70b7u,0x00000000u,0xc1ca8a76u,0xc0fd3a63u}},0x18fff3d7060e644bull},
 {3472731512u,1u,{{0x439cb519u,0x00000000u,0x443f8980u,0xc1074a8eu}},0x275b5bc71e553260ull},
 {1451931112u,2u,{{0x442108e7u,0x00000000u,0x43f106a9u,0x409cc6b6u}},0x0aa260d6fadae3bcull},
 {3608865860u,3u,{{0xc39e5009u,0x00000000u,0x43c50d6cu,0xc0c7f192u}},0xb4cdb6539a0b518full},
 {1939120316u,0u,{{0xc4271e19u,0x00000000u,0xc43947cau,0xbf3de1d8u}},0x6278c6ea80514c04ull},
 {3824114199u,1u,{{0x436b6c13u,0x00000000u,0x4438349bu,0xc08ce0b0u}},0x936b26ac27592a3bull},
 {3721253784u,2u,{{0xc3b11b75u,0x00000000u,0x44546437u,0xc0cdee23u}},0x0cc99579405a9cb5ull},
 {4188016765u,3u,{{0xc3ef6dc8u,0x00000000u,0x427ab8bfu,0xc0b26ffeu}},0x0bc4a36bb944e093ull},
}};
