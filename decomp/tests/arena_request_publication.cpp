#include "comet/arena_request_publication.hpp"
#include <cassert>
#include <numeric>
using namespace comet::decomp;
ArenaRequestPublicationQueue queue(unsigned capacity,unsigned available,unsigned head=0){
 ArenaRequestPublicationQueue q;q.records.resize(capacity);q.free_ring.resize(capacity);
 std::iota(q.free_ring.begin(),q.free_ring.end(),0);q.available=available;q.head=head;
 for(auto& r:q.records)r.fill(0xa5);return q;
}
int main(){
 ArenaCounterEmissionRequest r;r.descriptor.fill(0x7b);r.count=3;
 auto q=queue(160,131,159);q.availability_lanes={11,22,33};auto before=q;
 auto result=publish_arena_request(q,r);
 assert(result.status==ArenaRequestPublicationStatus::Published&&result.published==3);
 assert(q.available==128&&q.head==2&&q.free_ring==before.free_ring);
 assert((q.availability_lanes==std::array<std::uint32_t,3>{8,19,30}));
 for(unsigned id:{159u,0u,1u}){auto expected=r.descriptor;expected[12]=id>>8;expected[13]=id;assert(q.records[id]==expected);}
 assert(q.records[2]==before.records[2]);assert(r.descriptor[12]==0x7b);
 // Chunk reservation is atomic; a later exhausted chunk does not roll back32.
 q=queue(200,164);r.count=40;result=publish_arena_request(q,r);
 assert(result.status==ArenaRequestPublicationStatus::PoolFull&&result.published==32);
 assert(q.available==132&&q.head==32&&q.records[32][12]==0xa5);
 q=queue(160,130);before=q;r.count=3;result=publish_arena_request(q,r);
 assert(result.status==ArenaRequestPublicationStatus::PoolFull&&result.published==0);
 assert(q.records==before.records&&q.head==before.head&&q.available==before.available);
 // Distributed phase is continuous across the32-record chunk boundary.
 q=queue(200,200);r.count=64;r.mode=1;result=publish_arena_request(q,r);
 assert(result.status==ArenaRequestPublicationStatus::Published&&result.published==64);
 for(unsigned i=0;i<64;++i){assert(q.records[i][14]==i*255/64);assert(q.records[i][46]==i*255/64);}
 assert(q.records[63][14]==251&&q.head==64&&q.available==136);
 // Malformed native state is rejected before writes. Zero requests need no pool.
 q=queue(160,160);q.free_ring[2]=999;before=q;r.count=3;r.mode=0;
 assert(publish_arena_request(q,r).status==ArenaRequestPublicationStatus::Invalid);
 assert(q.records==before.records&&q.head==before.head&&q.available==before.available);
 ArenaRequestPublicationQueue empty;r.count=0;
 assert(publish_arena_request(empty,r).status==ArenaRequestPublicationStatus::Published);
 // Connect3510 to actual record publication, retaining source ignored-failure
 // semantics: the counter clears even if the publisher's reserve blocks a batch.
 q=queue(160,160);ArenaEntityWorkContext context{};context[0x35]=27;
 ArenaEmissionDescriptor scratch{};unsigned published=0;
 auto sink=[&](const ArenaCounterEmissionRequest& request){auto out=publish_arena_request(q,request);assert(out.status==ArenaRequestPublicationStatus::Published);published+=out.published;};
 assert(emit_arena_counter_requests(context,0xffffffffu,false,scratch,sink));
 assert(published==5&&context[0x35]==0&&q.available==155);
 assert(q.records[0][15]==72&&q.records[1][15]==72&&q.records[2][15]==73&&q.records[3][15]==74&&q.records[4][15]==74);
 q=queue(160,128);context[0x35]=27;published=0;
 auto exhausted=[&](const ArenaCounterEmissionRequest& request){auto out=publish_arena_request(q,request);assert(out.status==ArenaRequestPublicationStatus::PoolFull);published+=out.published;};
 assert(emit_arena_counter_requests(context,0xffffffffu,false,scratch,exhausted));
 assert(context[0x35]==0&&published==0&&q.available==128);
 // Original1E090/2AD8 fingerprints, including odd-count phase rounding and
 // nonascending FIFO identities across the aligned ring boundary.
 const std::array<unsigned,3> fixture_counts{33,65,129};
 const std::array<std::uint64_t,3> hashes{0x5b8a2890505187c9ull,0x17f5c71f2fb954cdull,0x993c9f208bccaec5ull};
 for(unsigned fixture=0;fixture<3;++fixture){
  q=queue(512,512,500);q.availability_lanes={0xabcdef12,0xabcdef13,0xabcdef14};
  for(unsigned i=0;i<512;++i)q.free_ring[i]=(i*73+17)%512;
  for(unsigned i=0;i<128;++i)r.descriptor[i]=std::uint8_t(i*37+19);
  r.count=fixture_counts[fixture];r.mode=1;
  assert(publish_arena_request(q,r).status==ArenaRequestPublicationStatus::Published);
  std::uint64_t hash=14695981039346656037ull;
  auto byte=[&](std::uint8_t v){hash=(hash^v)*1099511628211ull;};
  auto word=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)byte(std::uint8_t(v>>(24-8*i)));};
  word(q.head);word(q.available);for(auto v:q.availability_lanes)word(v);
  for(const auto& record:q.records)for(auto v:record)byte(v);
  assert(hash==hashes[fixture]);
 }
}
