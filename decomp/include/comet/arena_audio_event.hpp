#pragma once
#include "comet/arena_entity_accumulator.hpp"
namespace comet::decomp {
struct ArenaAudioEvent {std::uint16_t sound=0;std::uint8_t parameter_6=0,parameter_7=0;};
using ArenaAudioBatch=std::array<std::uint8_t,128>;
using ArenaAudioEventSink=std::function<void(const ArenaAudioEvent&)>;
enum class ArenaAudioAppend { Appended, Full, MissingTransport };
// B588's serialized128-byte batch mutation: count at0,28 four-byte entries at4.
// Full batches retain all bytes. This queues original events, not audio playback.
ArenaAudioAppend append_arena_audio_event(ArenaAudioBatch&,const ArenaAudioEvent&);
// B588 ->A9F8/A9B0: retry from a fresh reservation after a lost conditional
// write. The transport uses the same128-byte reservation protocol as1AD8.
// Caller supplies the original snapshot27EC queue address and platform ownership.
ArenaAudioAppend submit_arena_audio_event(ArenaAudioBatch& scratch,std::uint32_t address,
 const ArenaAudioEvent&,const ArenaAccumulatorTransport&);
}
