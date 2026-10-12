#include "comet/arena_factory_control.hpp"
namespace comet::decomp {
ArenaFactorySelection
select_arena_factory_production(std::span<std::uint8_t, 256> slot,
                                std::uint8_t selector) {
  if (selector < 4 || selector > 8 || (slot[15] != 26 && slot[15] != 28))
    return ArenaFactorySelection::Unsupported;
  const auto mode = static_cast<std::uint8_t>(selector - 4);
  if (slot[0xaa] == mode)
    return ArenaFactorySelection::Unchanged;
  for (unsigned i = 0x80; i < 0x84; ++i)
    slot[i] = 0;
  slot[0xaa] = mode;
  return ArenaFactorySelection::Changed;
}
} // namespace comet::decomp
