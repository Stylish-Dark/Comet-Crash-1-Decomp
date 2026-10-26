#include "comet/arena_audio_event.hpp"
namespace comet::decomp {
ArenaAudioAppend append_arena_audio_event(ArenaAudioBatch& batch,const ArenaAudioEvent& event){
 const auto count=std::uint32_t(batch[0])<<24|std::uint32_t(batch[1])<<16|std::uint32_t(batch[2])<<8|batch[3];
 if(count>27)return ArenaAudioAppend::Full;
 const unsigned offset=4+4*count;batch[offset]=std::uint8_t(event.sound>>8);batch[offset+1]=std::uint8_t(event.sound);
 batch[offset+2]=event.parameter_6;batch[offset+3]=event.parameter_7;
 for(unsigned i=0;i<4;++i)batch[i]=std::uint8_t((count+1)>>(24-8*i));
 return ArenaAudioAppend::Appended;
}
ArenaAudioAppend submit_arena_audio_event(ArenaAudioBatch& scratch,std::uint32_t address,
 const ArenaAudioEvent& event,const ArenaAccumulatorTransport& transport){
 if(!transport.read_reserved||!transport.write_conditional)return ArenaAudioAppend::MissingTransport;
 for(;;){
  auto fresh=transport.read_reserved(address);if(!fresh)return ArenaAudioAppend::MissingTransport;
  const auto appended=append_arena_audio_event(*fresh,event);
  const auto written=transport.write_conditional(address,*fresh);
  if(written==ArenaAccumulatorWrite::Retry)continue;
  if(written==ArenaAccumulatorWrite::Unavailable)return ArenaAudioAppend::MissingTransport;
  scratch=*fresh;return appended;
 }
}
}
