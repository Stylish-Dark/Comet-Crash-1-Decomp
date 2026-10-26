#include "comet/arena_particle_emission.hpp"
#include <bit>
#include <cassert>
using namespace comet::decomp;
namespace {
unsigned word(const auto& b,unsigned o){return unsigned(b[o])<<24|unsigned(b[o+1])<<16|unsigned(b[o+2])<<8|b[o+3];}
void put(auto& b,unsigned o,unsigned v){for(unsigned i=0;i<4;++i)b[o+i]=std::uint8_t(v>>(24-8*i));}
void fp(auto& b,unsigned o,float v){put(b,o,std::bit_cast<unsigned>(v));}
ArenaRequestPublicationQueue queue(unsigned available){
 ArenaRequestPublicationQueue q;q.records.resize(256);q.free_ring.resize(256);
 for(unsigned i=0;i<256;++i)q.free_ring[i]=255-i;
 q.head=252;q.available=available;return q;
}
ArenaParticleEmission request(){
 ArenaParticleEmission q;q.position={2.25f,1,3.5f,0};q.velocity={.3f,1,-.7f,0};
 q.radius_min=.03f;q.radius_max=.12f;q.speed=.13f;q.lifetime_min=.2f;q.lifetime_max=.3f;q.count=1;
 fp(q.configuration,0,.034f);fp(q.configuration,4,-2);
 q.configuration[8]=85;q.configuration[9]=3;q.configuration[11]=1;q.configuration[12]=1;return q;
}
}
int main(){
 auto q=request();auto pool=queue(129);ArenaRandom random(124);ArenaParticleEmissionScratch scratch;
 for(unsigned n=0;n<32;++n)for(unsigned i=0;i<128;++i)scratch.records[n][i]=std::uint8_t(17+n*13+i);
 auto result=publish_arena_particles(pool,q,random,scratch);
 assert(result.status==ArenaRequestPublicationStatus::Published&&result.published==1);
 assert(pool.head==253&&pool.available==128);
 // Original-instruction fixture covers all128 bytes, including preserved dirt.
 std::uint64_t hash=14695981039346656037ull;for(auto b:pool.records[3])hash=(hash^b)*1099511628211ull;
 assert(hash==0x50692d9369fde5e4ull&&random.next()==0x0f80c3cc);
 assert(pool.records[3][15]>=85&&pool.records[3][15]<=87);
 assert(word(pool.records[3],0x10)==4&&word(pool.records[3],0x18)==0xc0000000);
 assert(pool.records[3][0x1c]==0x2d&&pool.records[3][0x60]==0x71);
 // A failed batch consumes neither scratch bytes nor random numbers.
 const auto retained=pool.records;const auto retained_scratch=scratch.records;auto expected=random;
 result=publish_arena_particles(pool,q,random,scratch);
 assert(result.status==ArenaRequestPublicationStatus::PoolFull&&result.published==0);
 assert(pool.records==retained&&scratch.records==retained_scratch&&random.next()==expected.next());
 // One complete batch can publish before a later batch exhausts the reserve.
 pool=queue(160);q.count=33;random.seed(124);
 result=publish_arena_particles(pool,q,random,scratch);
 assert(result.status==ArenaRequestPublicationStatus::PoolFull&&result.published==32&&pool.available==128&&pool.head==28);
 for(auto lane:pool.availability_lanes)assert(lane==0xffffffe0u);
 // Directed-circle mode retains the supplied center and uses three draws/record.
 pool=queue(129);q=request();q.configuration[10]=1;random.seed(124);expected=random;
 for(unsigned i=0;i<3;++i)expected.next();
 result=publish_arena_particles(pool,q,random,scratch);
 assert(result.status==ArenaRequestPublicationStatus::Published&&random.next()==expected.next());
 for(unsigned i=0;i<3;++i)assert(word(pool.records[3],4*i)==std::bit_cast<unsigned>(q.position[i]));
 // Raw packed fourth-lane bits survive in the record's40 quadword.
 pool=queue(129);q.position[3]=std::bit_cast<float>(0x0003001du);q.configuration[10]=0;random.seed(124);
 assert(publish_arena_particles(pool,q,random,scratch).status==ArenaRequestPublicationStatus::Published);
 assert(word(pool.records[3],0x4c)==0x0003001d);
 // Invalid configuration rejects before pool/RNG/scratch mutation.
 pool=queue(129);q.configuration[9]=0;const auto before=pool.records;expected=random;
 assert(publish_arena_particles(pool,q,random,scratch).status==ArenaRequestPublicationStatus::Invalid);
 assert(pool.available==129&&pool.records==before&&random.next()==expected.next());
}
