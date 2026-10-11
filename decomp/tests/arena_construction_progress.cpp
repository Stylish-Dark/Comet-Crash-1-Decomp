#include "comet/arena_construction_simulation.hpp"
#include <bit>
#include <cassert>
#include <cfenv>
#include <cstdint>
#include <limits>
#include <stdexcept>
using namespace comet::decomp;
float scalar(const ArenaEntitySlot &slot, unsigned offset) {
  std::uint32_t word = 0;
  for (unsigned i = 0; i < 4; ++i)
    word = (word << 8) | slot[offset + i];
  return std::bit_cast<float>(word);
}
int main() {
  ArenaConstructionSimulationConfig config;
  config.capacity = 8;
  config.players = {{{{0, 0}, 0, false}, 400, 400}};
  for (unsigned type : {20, 21, 22, 23, 24, 25, 26, 28})
    config.tuning[type] = ArenaStructureTableValues{100, 50, 10, 20, 7.5f};
  ArenaConstructionSimulation simulation(config);
  assert(simulation.enqueue({0, 20, 2, 2}) == ArenaConstructionSubmit::Queued);
  assert(simulation.step().status == ArenaConstructionStep::Constructed);
  const auto original = simulation.storage().banks[0][0];
  assert(original[0x63] & 8);
  assert(original[0x63] & 128);
  simulation.advance(1);
  const auto &partial = simulation.storage().banks[0][0];
  assert(!(partial[0x63] & 128));
  assert(partial[0x63] & 8);
  assert(scalar(partial, 0x78) > .49f && scalar(partial, 0x78) < .51f);
  assert(partial[14] == 20);
  assert(scalar(partial, 0x38) > 51 && scalar(partial, 0x38) < 53);
  assert(simulation.advance(2).ready_entities == std::vector<std::uint16_t>{0});
  const auto ready = simulation.storage().banks[0][0];
  assert(!(ready[0x63] & 24));
  assert(ready[0x60] & 1);
  assert(scalar(ready, 0x74) == 0 && scalar(ready, 0x78) == 1);
  assert(ready[14] == 40 && scalar(ready, 0x38) == 100);
  assert(simulation.storage().banks[0][0] == simulation.storage().banks[1][0]);
  assert(simulation.accounts()[0].resource == 380);
  assert(simulation.advance(3).ready_entities.empty());
  assert(simulation.storage().banks[0][0] == ready);
  // A build created at the end of a tick starts aging on the next tick.
  assert(simulation.enqueue({0, 21, 3, 3}) == ArenaConstructionSubmit::Queued);
  assert(simulation.advance(4).construction.status ==
         ArenaConstructionStep::Constructed);
  assert(simulation.storage().banks[0][1][0x63] & 128);
  simulation.advance(5);
  assert(simulation.storage().banks[0][1][0x63] & 8);
  simulation.advance(6);
  assert(!(simulation.storage().banks[0][1][0x63] & 8));
  auto fixture = original;
  auto put = [&](unsigned offset, std::uint32_t bits) {
    for (unsigned i = 0; i < 4; ++i)
      fixture[offset + i] = static_cast<std::uint8_t>(bits >> (24 - 8 * i));
  };
  put(0x74, 0);
  put(0x78, 0);
  put(0x38, 0x40000000);
  const auto rounding = std::fegetround();
  assert(std::fesetround(FE_UPWARD) == 0);
  assert(advance_arena_structure_construction(fixture, 1) ==
         ArenaConstructionProgress::Building);
  assert(std::fegetround() == FE_UPWARD);
  assert(std::bit_cast<std::uint32_t>(scalar(fixture, 0x78)) == 0x3effffff);
  assert(std::bit_cast<std::uint32_t>(scalar(fixture, 0x38)) == 0x424fffff);
  assert(advance_arena_structure_construction(fixture, 1) ==
         ArenaConstructionProgress::Building);
  assert(std::bit_cast<std::uint32_t>(scalar(fixture, 0x78)) == 0x3f7fffff);
  assert(advance_arena_structure_construction(fixture, 1) ==
         ArenaConstructionProgress::Ready);
  assert(arena_structure_ready(fixture));
  assert(std::fesetround(rounding) == 0);
  for (float elapsed : {-1.0f, std::numeric_limits<float>::infinity(),
                        std::numeric_limits<float>::quiet_NaN()}) {
    const auto before = fixture;
    try {
      advance_arena_structure_construction(fixture, elapsed);
      assert(false);
    } catch (const std::invalid_argument &) {
    }
    assert(fixture == before);
  }
  fixture = original;
  fixture[0x63] |= 32;
  const auto blocked = fixture;
  assert(advance_arena_structure_construction(fixture, 1) ==
         ArenaConstructionProgress::Unsupported);
  assert(fixture == blocked);
  for (unsigned type : {20, 21, 22, 23, 24, 25, 26, 28}) {
    ArenaConstructionSimulation one(config);
    assert(one.enqueue({0, static_cast<std::uint8_t>(type), 2, 2}) ==
           ArenaConstructionSubmit::Queued);
    assert(one.step().status == ArenaConstructionStep::Constructed);
    assert(one.advance(2).ready_entities == std::vector<std::uint16_t>{0});
    assert(arena_structure_ready(one.storage().banks[0][0]));
  }
}
