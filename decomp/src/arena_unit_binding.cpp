#include "comet/arena_unit_binding.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
// The original ground branch only accepts these eight fixed directions from
// 20AF0. Quaternion rows are the exact 2400 outputs for their negated vectors,
// avoiding a host atan/sin approximation (including original signed zero).
constexpr std::array<std::array<std::uint32_t, 3>, 8> directions{
    {{0x3f3504f3, 0, 0x3f3504f3},
     {0, 0, 0x3f800000},
     {0xbf3504f3, 0, 0x3f3504f3},
     {0xbf800000, 0, 0},
     {0xbf3504f3, 0, 0xbf3504f3},
     {0, 0, 0xbf800000},
     {0x3f3504f3, 0, 0xbf3504f3},
     {0x3f800000, 0, 0}}};
constexpr std::array<std::array<std::uint32_t, 4>, 8> headings{
    {{0, 0xbf6c835e, 0, 0x3ec3ef16},
     {0, 0x3f7fffff, 0, 0xb33bbd2f},
     {0, 0x3f6c835e, 0x80000000, 0x3ec3ef16},
     {0, 0x3f3504f3, 0x80000000, 0x3f3504f3},
     {0, 0x3ec3ef16, 0, 0x3f6c835f},
     {0, 0x80000000, 0, 0x3f800000},
     {0, 0xbec3ef16, 0, 0x3f6c835f},
     {0, 0xbf3504f3, 0, 0x3f3504f3}}};
std::uint32_t word(std::span<const std::uint8_t, 256> s, unsigned off) {
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v = (v << 8) | s[off + i];
  return v;
}
void put(std::span<std::uint8_t, 256> s, unsigned off, std::uint32_t v) {
  for (unsigned i = 0; i < 4; ++i)
    s[off + i] = v >> (24 - 8 * i);
}
struct Rounding {
  int previous = std::fegetround();
  Rounding() {
    if (previous < 0 || std::fesetround(FE_TOWARDZERO) != 0)
      throw std::runtime_error("unit binding rounding unavailable");
  }
  ~Rounding() { std::fesetround(previous); }
};
} // namespace
std::optional<std::array<float,3>> arena_ground_route_vector(std::uint8_t direction) {
  if(direction>=directions.size())return {};
  std::array<float,3> result{};
  for(unsigned i=0;i<3;++i)result[i]=std::bit_cast<float>(directions[direction][i]);
  return result;
}
std::optional<std::array<float,4>> arena_ground_route_heading(std::uint8_t direction) {
  if(direction>=headings.size())return {};
  std::array<float,4> result{};
  for(unsigned i=0;i<4;++i)result[i]=std::bit_cast<float>(headings[direction][i]);
  return result;
}
ArenaGroundUnitBinding
bind_arena_ground_unit_template(std::span<std::uint8_t, 256> slot,
                                std::span<const ArenaUnitTargetPlayer> players,
                                const ArenaRoutingGrid &grid,
                                ArenaRandom &random) {
  const unsigned owner = slot[0x33] >> 5;
  const unsigned x = (slot[0xc8] << 8) | slot[0xc9],
                 z = (slot[0xca] << 8) | slot[0xcb];
  const float speed = std::bit_cast<float>(word(slot, 0x84));
  if (players.empty() || players.size() > 4 || owner >= players.size() ||
      slot[15] < 40 || slot[15] > 48 || slot[15] == 42 || slot[15] == 48 ||
      (slot[0x33] & 28) != 20 || x >= 24 || z >= 24 || !std::isnormal(speed) ||
      speed <= 0)
    return ArenaGroundUnitBinding::Unsupported;
  const auto forced = players[owner].override_target;
  // Self-target overrides do not index a valid directed pair in the native
  // 12-route cell. Reject this malformed native state before writes/draws.
  if (forced != 255 && (forced >= players.size() || forced == owner))
    return ArenaGroundUnitBinding::Unsupported;
  Rounding rounding;
  std::uint8_t target = 255;
  if (forced != 255 && players[forced].base_available)
    target = forced;
  else {
    std::array<std::uint8_t, 4> candidates{};
    unsigned count = 0;
    for (unsigned p = 0; p < players.size(); ++p)
      if (players[p].team != players[owner].team && players[p].base_available)
        candidates[count++] = static_cast<std::uint8_t>(p);
    if (count)
      target = candidates[random.next() % count];
  }
  slot[0x7e] = target;
  if (target == 255)
    return ArenaGroundUnitBinding::NoTarget;
  const auto pair = 3 * owner + target - (target > owner ? 1 : 0);
  const auto route = grid.cells[z * 24 + x].routes[pair];
  if (route & 1)
    return ArenaGroundUnitBinding::Blocked;
  const auto direction = (route >> 1) & 7;
  for (unsigned axis = 0; axis < 3; ++axis) {
    volatile float component =
        std::bit_cast<float>(directions[direction][axis]) * speed;
    float value = std::fpclassify(component) == FP_SUBNORMAL ? 0.0f : component;
    put(slot, 0x40 + 4 * axis, std::bit_cast<std::uint32_t>(value));
  }
  put(slot, 0x4c, 0);
  for (unsigned axis = 0; axis < 4; ++axis)
    put(slot, 0x20 + 4 * axis, headings[direction][axis]);
  return ArenaGroundUnitBinding::Bound;
}
} // namespace comet::decomp
