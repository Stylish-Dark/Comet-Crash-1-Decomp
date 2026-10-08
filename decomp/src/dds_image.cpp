#include "comet/dds_image.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <utility>
namespace comet::decomp {
namespace {
std::uint32_t u32(std::span<const std::uint8_t>b,std::size_t o){return b[o]|std::uint32_t(b[o+1])<<8|std::uint32_t(b[o+2])<<16|std::uint32_t(b[o+3])<<24;}
std::uint16_t u16(std::span<const std::uint8_t>b,std::size_t o){return b[o]|std::uint16_t(b[o+1])<<8;}
using Color=std::array<std::uint8_t,4>;
Color rgb565(std::uint16_t value){return {static_cast<std::uint8_t>(((value>>11)&31)*255/31),static_cast<std::uint8_t>(((value>>5)&63)*255/63),static_cast<std::uint8_t>((value&31)*255/31),255};}
std::uint8_t channel(std::uint32_t pixel,std::uint32_t mask,std::uint8_t fallback){
 if(!mask)return fallback;
 const auto shift=std::countr_zero(mask);const auto maximum=mask>>shift;
 return static_cast<std::uint8_t>((std::uint64_t((pixel&mask)>>shift)*255)/maximum);
}
AssetLoadResult bad(std::string detail){return {false,0,std::move(detail)};}
}
AssetLoadResult decode_dds_image(std::span<const std::uint8_t>b,DdsImage& output){
 if(b.size()<128||u32(b,0)!=0x20534444||u32(b,4)!=124||u32(b,76)!=32)return bad("invalid DDS header");
 const auto width=u32(b,16),height=u32(b,12),flags=u32(b,80),fourcc=u32(b,84),bits=u32(b,88);
 if(!width||!height||width>8192||height>8192||std::uint64_t(width)*height>16777216)return bad("DDS dimensions exceed limits");
 if(u32(b,112)&(0x200|0x200000))return bad("cube/volume DDS requires a separate loader");
 DdsImage image;image.width=width;image.height=height;image.rgba.resize(static_cast<std::size_t>(width)*height*4);
 if(flags&4){
  const bool dxt1=fourcc==0x31545844,dxt3=fourcc==0x33545844,dxt5=fourcc==0x35545844;
  if(!dxt1&&!dxt3&&!dxt5)return bad("unsupported DDS compression");
  const std::size_t block_size=dxt1?8:16,blocks_x=(width+3)/4,blocks_y=(height+3)/4;
  if(b.size()-128<blocks_x*blocks_y*block_size)return bad("truncated DDS blocks");
  for(std::size_t y=0;y<blocks_y;++y)for(std::size_t x=0;x<blocks_x;++x){
   const auto start=128+(y*blocks_x+x)*block_size;const auto color_start=start+(dxt1?0:8);
   const auto c0=u16(b,color_start),c1=u16(b,color_start+2);
   std::array<Color,4> colors{rgb565(c0),rgb565(c1),{}, {}};
   for(int c=0;c<3;++c){
    if(c0>c1||!dxt1){colors[2][c]=(2*colors[0][c]+colors[1][c])/3;colors[3][c]=(colors[0][c]+2*colors[1][c])/3;}
    else{colors[2][c]=(colors[0][c]+colors[1][c])/2;colors[3][c]=0;}
   }
   colors[2][3]=255;colors[3][3]=(c0>c1||!dxt1)?255:0;
   const auto selectors=u32(b,color_start+4);
   std::array<std::uint8_t,8> alpha{};std::uint64_t alpha_selectors=0;
   if(dxt5){alpha[0]=b[start];alpha[1]=b[start+1];
    if(alpha[0]>alpha[1])for(int i=2;i<8;++i)alpha[i]=((8-i)*alpha[0]+(i-1)*alpha[1])/7;
    else{for(int i=2;i<6;++i)alpha[i]=((6-i)*alpha[0]+(i-1)*alpha[1])/5;alpha[6]=0;alpha[7]=255;}
    for(int i=0;i<6;++i)alpha_selectors|=std::uint64_t(b[start+2+i])<<(8*i);
   }
   for(int i=0;i<16;++i){
    const auto px=x*4+i%4,py=y*4+i/4;if(px>=width||py>=height)continue;
    auto color=colors[(selectors>>(i*2))&3];
    if(dxt5)color[3]=alpha[(alpha_selectors>>(i*3))&7];
    if(dxt3)color[3]=((b[start+i/2]>>((i%2)*4))&15)*17;
    for(int c=0;c<4;++c)image.rgba[(py*width+px)*4+c]=color[c];
   }
  }
 }else{
  if((bits!=8&&bits!=16&&bits!=24&&bits!=32)||!(flags&(0x40|0x20000)))return bad("unsupported DDS pixel format");
  const auto bytes_per_pixel=bits/8;
  std::size_t stride=static_cast<std::size_t>(width)*bytes_per_pixel;
  if(u32(b,8)&8){const auto pitch=u32(b,20);if(pitch<stride)return bad("invalid DDS row pitch");stride=pitch;}
  if(stride> (b.size()-128)/height)return bad("truncated DDS pixels");
  const auto red=u32(b,92),green=u32(b,96),blue=u32(b,100),alpha=u32(b,104);
  for(std::size_t y=0;y<height;++y)for(std::size_t x=0;x<width;++x){
   std::uint32_t pixel=0;for(std::size_t c=0;c<bytes_per_pixel;++c)pixel|=std::uint32_t(b[128+y*stride+x*bytes_per_pixel+c])<<(c*8);
   const auto index=(y*width+x)*4;const auto r=channel(pixel,red,0);
   image.rgba[index]=r;image.rgba[index+1]=(flags&0x20000)?r:channel(pixel,green,0);image.rgba[index+2]=(flags&0x20000)?r:channel(pixel,blue,0);image.rgba[index+3]=channel(pixel,alpha,255);
  }
 }
 output=std::move(image);return {};
}
AssetLoadResult load_dds_image(const std::filesystem::path& path,DdsImage& output){
 std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input)return bad("cannot open texture: "+path.string());
 const auto size=input.tellg();if(size<128||size>128*1024*1024)return bad("invalid DDS file size: "+path.string());
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));input.seekg(0);input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!input)return bad("DDS read failure: "+path.string());
 auto result=decode_dds_image(bytes,output);if(!result)result.detail=path.string()+": "+result.detail;return result;
}
}
