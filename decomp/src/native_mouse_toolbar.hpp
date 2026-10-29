#pragma once
#include "comet/native_mouse_input.hpp"
#include "native_status_overlay.hpp"
namespace comet::decomp::detail {
inline constexpr std::array<unsigned,8> mouse_build_types{20,21,22,23,24,25,26,28};
inline void draw_mouse_toolbar(int width,int height,int drawable_width,int drawable_height,
 unsigned opcode,bool playing,bool finished,bool controllable,bool releasing,bool pilot,int hover){
 if(width<32||height<120)return;
 glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT|GL_DEPTH_BUFFER_BIT|GL_VIEWPORT_BIT);
 glViewport(0,0,drawable_width,drawable_height);
 glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glDisable(GL_BLEND);
 glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
 glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 auto quad=[](int x,int y,int w,int h){glVertex2i(x,y);glVertex2i(x+w,y);glVertex2i(x+w,y+h);glVertex2i(x,y+h);};
 glBegin(GL_QUADS);glColor3f(.025f,.035f,.055f);quad(0,height-110,width,110);
 const std::array<std::string,15> labels{"BEACON","CANNON","TESLA","LASER","MISSILE","GATE","FACTORY","ADV FACTORY",
     playing?"PAUSE":"PLAY","RESTART","HOLD RELEASE","PREV UNIT","NEXT UNIT","FACTORY MODE",pilot?"GRID":"SHIP"};
 for(unsigned i=0;i<labels.size();++i){
  const auto r=native_mouse_button(i,width,height);
  const bool enabled=i==9||(!finished&&(i==8||controllable));
  const bool selected=(i<8&&opcode==mouse_build_types[i])||(i==10&&releasing)||(i==14&&pilot);
  if(!enabled)glColor3f(.075f,.085f,.105f);
  else if(selected)glColor3f(.08f,.32f,.55f);
  else if(int(i)==hover)glColor3f(.22f,.32f,.43f);
  else glColor3f(.12f,.18f,.26f);
  quad(r.x,r.y,r.w,r.h);
  const int scale=r.w>=130?2:1,columns=std::max(0,(r.w-10)/(6*scale));
  const auto label=labels[i].substr(0,std::size_t(columns));
  const int left=r.x+(r.w-int(label.size())*6*scale)/2,top=r.y+(r.h-7*scale)/2;
  if(enabled)glColor3f(.93f,.96f,1.f);else glColor3f(.4f,.45f,.5f);
  for(unsigned c=0;c<label.size();++c){const auto glyph=status_glyph(label[c]);
   for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(glyph[y]&(1u<<(4-x)))quad(left+(int(c)*6+x)*scale,top+y*scale,scale,scale);
  }
 }
 glEnd();glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopAttrib();
}
}
