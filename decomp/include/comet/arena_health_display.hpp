#pragma once
#include "comet/arena_entity_storage.hpp"
#include "comet/arena_request_publication.hpp"
namespace comet::decomp {
struct ArenaHealthDisplayParameters {
 std::uint32_t kind_500=0, flags_520=0;
 float offset_4e4=0;
};
using ArenaHealthDisplayScratch=std::array<ArenaEmissionDescriptor,2>;
struct ArenaHealthDisplayTransport {
 ArenaRequestPublicationQueue* queue=nullptr;
 ArenaHealthDisplayScratch* scratch=nullptr;
};
// Original FE5D8 3940 ->1E480: two kind3 display records, FIFO reserve1024.
// Already-owned displays do nothing. PoolFull is a completed original call:
// body7F remains0 and the caller must still commit the updated health.
// Uses source cached kind/flags/offset, position480 and reused380/400 bytes.
ArenaRequestPublicationResult publish_arena_health_display(
 ArenaRequestPublicationQueue&,ArenaEntitySlot&,const ArenaHealthDisplayParameters&,
 const std::array<float,4>& position_480,ArenaHealthDisplayScratch&);
}
