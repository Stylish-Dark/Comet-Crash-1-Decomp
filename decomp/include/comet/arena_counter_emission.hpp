#pragma once
#include "comet/arena_entity_preparation.hpp"
#include <functional>
namespace comet::decomp {
using ArenaEmissionDescriptor=std::array<std::uint8_t,128>;
struct ArenaCounterEmissionRequest {
 ArenaEmissionDescriptor descriptor{};
 std::uint32_t count=0;
 std::uint8_t mode=0; // source1E090 r5
};
using ArenaCounterEmissionSink=std::function<void(const ArenaCounterEmissionRequest&)>;
// Original3510..387C. Model72/73/74 counts are n%5,(n%10)/5,n/10,
// with scalars1/5/10. Optional model76 has count1/scalar0. This does not guess
// model meanings. Source1E090 publication is a required dependency when a
// request exists. Sink must not throw/re-enter/mutate context or descriptor.
// Reused descriptor retains all unwritten bytes, just as source scratch does.
// Clear body+34 only after ordered requests; missing sink preserves both inputs.
bool emit_arena_counter_requests(ArenaEntityWorkContext&,std::uint32_t attribution,
 bool optional_request,ArenaEmissionDescriptor& reused,const ArenaCounterEmissionSink&);
}
