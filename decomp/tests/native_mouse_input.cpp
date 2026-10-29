#include "comet/native_mouse_input.hpp"
#include <cassert>
#include <cmath>
#include <algorithm>
#include <utility>
#include <limits>
using namespace comet::decomp;
namespace {
std::pair<int,int> project(double x,double z,int width,int height,unsigned extent,double yaw,double pitch,double zoom){
 constexpr double r=3.14159265358979323846/180;
 x-=extent*.5;z-=extent*.5;
 const double vx=std::cos(yaw*r)*x+std::sin(yaw*r)*z;
 const double rotated_z=-std::sin(yaw*r)*x+std::cos(yaw*r)*z;
 const double vy=-std::sin(pitch*r)*rotated_z;
 const double aspect=double(width)/height;
 const double depth=extent*.6*3.2*zoom/std::min(1.,aspect)-std::cos(pitch*r)*rotated_z;
 return {int(std::lround((1+vx/depth/std::tan(22.5*r)/aspect)*width*.5)),
         int(std::lround((1-vy/depth/std::tan(22.5*r))*height*.5))};
}
}
int main(){
 // Forward-project independent ground centers, then recover their cells.
 for(auto size:{std::pair{1280,720},std::pair{960,640},std::pair{600,900}})
 for(float yaw:{-90.f,0.f,25.f,120.f})for(float pitch:{30.f,55.f,80.f})for(float zoom:{.7f,1.f,1.8f})
 for(auto cell:{ArenaGridPosition{2,3},ArenaGridPosition{11,12},ArenaGridPosition{20,19}}){
  const auto [x,y]=project(cell.x+.5,cell.z+.5,size.first,size.second,24,yaw,pitch,zoom);
  if(x<0||y<0||x>=size.first||y>=size.second)continue;
  const auto picked=native_mouse_cell(x,y,size.first,size.second,24,yaw,pitch,zoom);
  assert(picked&&*picked==cell);
 }
 assert(!native_mouse_cell(-1,10,1280,720,24,25,55,1));
 assert(!native_mouse_cell(10,10,0,720,24,25,55,1));
 assert(!native_mouse_cell(10,10,1280,720,24,25,55,std::numeric_limits<float>::quiet_NaN()));
 assert(!native_mouse_cell(640,360,1280,720,24,0,0,1));
 // Toolbar hit regions stay distinct at narrow and large window sizes.
 for(int width:{320,800,1280,2560})for(unsigned i=0;i<15;++i){
  const auto b=native_mouse_button(i,width,720);
  assert(b.w>0&&b.h>0&&native_mouse_button_at(b.x+b.w/2,b.y+b.h/2,width,720)==int(i));
  assert(native_mouse_toolbar_at(b.y,720));
 }
 assert(native_mouse_button_at(100,100,1280,720)==-1);
}
