#include "comet/arena_factory_control.hpp"
#include <cassert>
using namespace comet::decomp;
int main() {
  for (unsigned type : {26, 28}) {
    for (unsigned current = 0; current < 5; ++current) {
      for (unsigned selector = 4; selector <= 8; ++selector) {
        ArenaEntitySlot slot;
        slot.fill(0xa5);
        slot[15] = type;
        slot[0xaa] = current;
        auto expected = slot;
        const bool changed = current != selector - 4;
        if (changed) {
          expected[0xaa] = selector - 4;
          for (unsigned i = 0x80; i < 0x84; ++i)
            expected[i] = 0;
        }
        assert(select_arena_factory_production(slot, selector) ==
               (changed ? ArenaFactorySelection::Changed
                        : ArenaFactorySelection::Unchanged));
        assert(slot == expected);
      }
    }
  }
  for (unsigned type : {0, 20, 21, 22, 23, 24, 25, 27, 29, 63, 255}) {
    ArenaEntitySlot slot{};
    slot[15] = type;
    auto before = slot;
    assert(select_arena_factory_production(slot, 4) ==
           ArenaFactorySelection::Unsupported);
    assert(slot == before);
  }
  for (unsigned selector : {0, 1, 2, 3, 9, 10, 11, 255}) {
    ArenaEntitySlot slot{};
    slot[15] = 26;
    auto before = slot;
    assert(select_arena_factory_production(slot, selector) ==
           ArenaFactorySelection::Unsupported);
    assert(slot == before);
  }
}
