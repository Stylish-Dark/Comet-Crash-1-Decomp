#include "comet/dds_image.hpp"
#include <stdexcept>
using namespace comet::decomp;
void check(bool b){if(!b)throw std::runtime_error("DDS check failed");}
void put(std::vector<std::uint8_t>&b,int o,std::uint32_t n){for(int i=0;i<4;++i)b[o+i]=n>>(i*8);}
std::vector<std::uint8_t> header(int width,int height){std::vector<std::uint8_t>b(128);b[0]='D';b[1]='D';b[2]='S';b[3]=' ';put(b,4,124);put(b,12,height);put(b,16,width);put(b,76,32);return b;}
int main(){
 auto b=header(2,1);put(b,80,0x41);put(b,88,32);put(b,92,0xff0000);put(b,96,0xff00);put(b,100,0xff);put(b,104,0xff000000);b.insert(b.end(),{3,2,1,255,6,5,4,128});DdsImage image;
 check(bool(decode_dds_image(b,image)));check(image.width==2&&image.height==1);check(image.rgba[0]==1&&image.rgba[2]==3&&image.rgba[7]==128);
 auto d=header(4,4);put(d,80,4);put(d,84,0x31545844);d.insert(d.end(),{0,0xf8,0,0,0,0,0,0});check(bool(decode_dds_image(d,image)));check(image.rgba[0]==255&&image.rgba[1]==0&&image.rgba[3]==255);
 auto d5=header(4,4);put(d5,80,4);put(d5,84,0x35545844);d5.insert(d5.end(),{128,0,0,0,0,0,0,0,0,0xf8,0,0,0,0,0,0});check(bool(decode_dds_image(d5,image)));check(image.rgba[0]==255&&image.rgba[3]==128);
 d5.resize(135);check(!decode_dds_image(d5,image));check(image.rgba[3]==128);
 auto lum=header(1,1);put(lum,80,0x20001);put(lum,88,16);put(lum,92,0xff);put(lum,104,0xff00);lum.insert(lum.end(),{40,200});check(bool(decode_dds_image(lum,image)));check(image.rgba==std::vector<std::uint8_t>({40,40,40,200}));
 auto bad=header(100000,100000);check(!decode_dds_image(bad,image));
}
