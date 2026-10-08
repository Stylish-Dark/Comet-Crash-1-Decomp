#include "comet/native_viewer.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <array>
using namespace comet::decomp;
int main() {
    NativeViewerConfig config;config.width=320;config.height=240;config.frame_limit=2;config.hidden=true;
    config.screenshot=std::filesystem::temp_directory_path()/"comet-render-test.ppm";
    ArenaNativeModel m{arena_model_asset_manifest()[0],{}};
    m.model.vertices={{{-1,-1,0},{0,0,1},0,0},{{1,-1,0},{0,0,1},1,0},{{0,1,0},{0,0,1},0.5f,1}};
    m.model.indices={0,1,2};NativeSubmesh mesh;mesh.range={0,3,0,2};mesh.material.properties.diffuse={0.9f,0.3f,0.1f,0};m.model.submeshes={mesh};m.model.bounds_min={-1,-1,0};m.model.bounds_max={1,1,0};
    // A green texture over a red material proves the graphics path samples DDS.
    auto texture=config.screenshot;texture.replace_extension(".dds");
    std::array<unsigned char,132> dds{};auto put=[&](int offset,unsigned value){for(int i=0;i<4;++i)dds[offset+i]=value>>(i*8);};
    put(0,0x20534444);put(4,124);put(12,1);put(16,1);put(76,32);put(80,0x41);put(88,32);put(92,0xff);put(96,0xff00);put(100,0xff0000);put(104,0xff000000);dds[129]=255;dds[131]=255;
    {std::ofstream out(texture,std::ios::binary);out.write(reinterpret_cast<char*>(dds.data()),dds.size());}
    m.model.submeshes[0].material.diffuse_texture=texture;
    const auto result=run_native_viewer({m},config);
    if (!result) throw std::runtime_error(result.detail);
    std::ifstream input(config.screenshot,std::ios::binary);std::string magic;int width,height,max;input>>magic>>width>>height>>max;input.get();
    if (magic!="P6"||width!=320||height!=240||max!=255) throw std::runtime_error("invalid rendered screenshot");
    std::vector<unsigned char> pixels(320*240*3);input.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
    int colored=0;for(std::size_t i=0;i<pixels.size();i+=3)if(pixels[i+1]>pixels[i]*2 && pixels[i+1]>60)++colored;
    if(colored<100)throw std::runtime_error("model triangles not visible");
    std::filesystem::remove(config.screenshot);std::filesystem::remove(texture);
}
