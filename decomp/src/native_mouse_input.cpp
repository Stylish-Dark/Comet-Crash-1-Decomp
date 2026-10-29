#include "comet/native_mouse_input.hpp"
#include <cmath>
#include <algorithm>
namespace comet::decomp {
std::optional<ArenaGridPosition> native_mouse_cell(int x,int y,int width,int height,
 unsigned extent,float yaw,float pitch,float zoom){
 if(width<=0||height<=0||x<0||y<0||x>=width||y>=height||!extent||extent>24||
    !std::isfinite(yaw)||!std::isfinite(pitch)||!std::isfinite(zoom)||zoom<=0)return {};
 constexpr double radians=3.14159265358979323846/180;
 const double sy=std::sin(yaw*radians),cy=std::cos(yaw*radians);
 const double sp=std::sin(pitch*radians),cp=std::cos(pitch*radians);
 const double aspect=double(width)/height;
 const double distance=extent*.6*3.2*zoom/std::min(1.,aspect);
 // Invert T(-distance) Rx(pitch) Ry(yaw) T(-arena center).
 const double ox=extent*.5-sy*cp*distance,oy=sp*distance,oz=extent*.5+cy*cp*distance;
 const double vx=(2.*x/width-1)*std::tan(22.5*radians)*aspect;
 const double vy=(1-2.*y/height)*std::tan(22.5*radians);
 const double ry=cp*vy-sp,rz=-sp*vy-cp;
 if(std::abs(ry)<1e-10)return {};
 const double t=-oy/ry;if(t<=0)return {};
 const double wx=ox+t*(cy*vx-sy*rz),wz=oz+t*(sy*vx+cy*rz);
 if(!std::isfinite(wx)||!std::isfinite(wz)||wx<0||wz<0||wx>=extent||wz>=extent)return {};
 return ArenaGridPosition{std::uint8_t(std::floor(wx)),std::uint8_t(std::floor(wz))};
}
NativeMouseRect native_mouse_button(unsigned index,int width,int height){
 if(index>=15||width<32||height<120)return {};
 const int columns=index<8?8:7,slot=index<8?int(index):int(index)-8;
 const int gap=4,left=8,available=width-2*left;
 const int x=left+slot*available/columns,w=(slot+1)*available/columns-slot*available/columns-gap;
 return {x,height-(index<8?94:48),w,36};
}
int native_mouse_button_at(int x,int y,int width,int height){
 for(unsigned i=0;i<15;++i)if(native_mouse_button(i,width,height).contains(x,y))return int(i);
 return -1;
}
bool native_mouse_toolbar_at(int y,int height){return height>=120&&y>=height-110;}
}
