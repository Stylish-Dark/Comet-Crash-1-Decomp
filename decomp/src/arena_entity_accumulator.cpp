#include "comet/arena_entity_accumulator.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace comet::decomp {
namespace {
std::uint32_t word(const ArenaAccumulatorRecord& b,unsigned o){
 return std::uint32_t(b[o])<<24|std::uint32_t(b[o+1])<<16|std::uint32_t(b[o+2])<<8|b[o+3];
}
void put(ArenaAccumulatorRecord& b,unsigned o,std::uint32_t v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
float scalar(const ArenaAccumulatorRecord& b,unsigned o){return std::bit_cast<float>(word(b,o));}
bool number(float f){return std::isfinite(f)&&std::fpclassify(f)!=FP_SUBNORMAL;}
bool valid(const ArenaAccumulatorRecord& b){
 for(unsigned o:{0x10,0x40,0x44,0x48})if(!number(scalar(b,o)))return false;
 return true;
}
struct Rounding{
 int previous=std::fegetround();
 Rounding(){if(previous<0||std::fesetround(FE_TOWARDZERO)!=0)throw std::runtime_error("accumulator rounding unavailable");}
 ~Rounding(){std::fesetround(previous);}
};
ArenaAccumulatorConsumption output(const ArenaAccumulatorRecord& b,ArenaAccumulatorStatus status,unsigned attempts){
 ArenaAccumulatorConsumption r;r.status=status;r.attempts=attempts;
 std::copy_n(b.begin()+0x10,16,r.health_10.begin());
 std::copy_n(b.begin()+0x40,16,r.impulse_40.begin());r.request_30=word(b,0x30);return r;
}
ArenaAccumulatorRecord cleared(ArenaAccumulatorRecord b,std::uint32_t target){
 // 1CEC..1E6C: clear exactly these consumed lanes, retaining other bytes.
 for(unsigned o:{0x10,0x20,0x40,0x50})std::fill_n(b.begin()+o,16,0);
 std::fill_n(b.begin()+0x30,16,255);
 put(b,0,target);for(unsigned o:{4,8,0x60,0x70})put(b,o,0);
 std::fill_n(b.begin()+0xc,4,0);put(b,0x74,0x42c80000);put(b,0x78,0xffffffff);
 return b;
}
}
ArenaAccumulatorConsumption consume_arena_entity_accumulator(ArenaAccumulatorRecord& local,
 const ArenaAccumulatorTransport& transport){
 if(!valid(local))return {};
 Rounding rounding;
 volatile float squared=scalar(local,0x40)*scalar(local,0x40);
 const float partial=std::fma(scalar(local,0x44),scalar(local,0x44),squared);
 const float speed_squared=std::fma(scalar(local,0x48),scalar(local,0x48),partial);
 if(!number(squared)||!number(partial)||!number(speed_squared))return {};
 // 1C20..1CBC: signed counter gates, raw word gates and -1 sentinel.
 const bool pending=scalar(local,0x10)!=0 || std::bit_cast<std::int32_t>(word(local,0x50))>0 ||
   word(local,0x60)!=0 || speed_squared>0 || std::bit_cast<std::int32_t>(word(local,0x70))>0 ||
   word(local,4)!=0 || word(local,8)!=0 || word(local,0x78)!=0xffffffffu;
 const auto target=word(local,0);
 if(!target||!pending)return output(local,ArenaAccumulatorStatus::Cached,0);
 if(!transport.read_reserved||!transport.write_conditional)
  return output(local,ArenaAccumulatorStatus::MissingTransport,0);
 unsigned attempts=0;
 for(;;){
  // Every failed PUTLLC retries GETLLAR and snapshots fresh values. Never
  // clear/return the stale pre-call snapshot or aggregate it a second time.
  auto fresh=transport.read_reserved(target);++attempts;
  if(!fresh)return output(local,ArenaAccumulatorStatus::MissingTransport,attempts);
  if(!valid(*fresh))return output(local,ArenaAccumulatorStatus::Invalid,attempts);
  const auto status=transport.write_conditional(target,cleared(*fresh,target));
  if(status==ArenaAccumulatorWrite::Retry)continue;
  if(status==ArenaAccumulatorWrite::Unavailable)
   return output(local,ArenaAccumulatorStatus::MissingTransport,attempts);
  // 1E88..1F60 restore the fresh consumed snapshot in local storage after
  // clearing shared memory. The following caller reads that local data.
  local=*fresh;return output(local,ArenaAccumulatorStatus::Consumed,attempts);
 }
}
ArenaAccumulatorConsumption consume_arena_prepared_accumulator(ArenaEntityWorkContext& context,
 const ArenaAccumulatorTransport& transport){
 ArenaAccumulatorRecord local;std::copy_n(context.begin()+0x300,128,local.begin());
 auto result=consume_arena_entity_accumulator(local,transport);
 if(result.status==ArenaAccumulatorStatus::Consumed)
  std::copy(local.begin(),local.end(),context.begin()+0x300);
 return result;
}
}
