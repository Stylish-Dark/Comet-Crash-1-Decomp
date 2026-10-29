#pragma once
#include <SDL_opengl.h>
#include <algorithm>
#include <array>
#include <string>
namespace comet::decomp::detail {
// Small native bitmap alphabet: no dependency on title fonts or platform fonts.
inline std::array<unsigned,7> status_glyph(char c){
 switch(c){
 case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
 case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
 case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
 case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
 case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};
 case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
 case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
 case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
 case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
 case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
 case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
 case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
 case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
 case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
 case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
 case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
 case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
 case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
 case ':':return {0,4,4,0,4,4,0};case '-':return {0,0,0,31,0,0,0};
 case '/':return {1,2,2,4,8,8,16};case '.':return {0,0,0,0,0,4,4};
 default:return {};
 }
}
inline int status_overlay_height(const std::array<std::string,5>& lines,int width){
 const int scale=width>=800?2:1,columns=std::max(1,(width-20)/(6*scale));
 int rows=0;for(const auto& line:lines)rows+=std::max(1,(int(line.size())+columns-1)/columns);
 return 16+rows*10*scale;
}
inline void draw_status_overlay(const std::array<std::string,5>&lines,int width,int height){
 const int scale=width>=800?2:1;
 const int columns=std::max(1,(width-20)/(6*scale));
 int rows=0;for(const auto &line:lines)rows+=std::max(1,(static_cast<int>(line.size())+columns-1)/columns);
 glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT|GL_DEPTH_BUFFER_BIT|GL_VIEWPORT_BIT);
 glViewport(0,0,width,height);
 glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glDisable(GL_BLEND);
 glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
 glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 auto quad=[](int x,int y,int w,int h){glVertex2i(x,y);glVertex2i(x+w,y);glVertex2i(x+w,y+h);glVertex2i(x,y+h);};
 glBegin(GL_QUADS);glColor3f(.025f,.035f,.055f);quad(4,4,width-8,std::min(height-8,12+rows*10*scale));
 glColor3f(.9f,.95f,1.f);int row=0;
 for(const auto &line:lines){
  int column=0;
  for(char c:line){
   if(column==columns){column=0;++row;}
   const auto glyph=status_glyph(c);
   for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(glyph[y]&(1u<<(4-x)))quad(10+(column*6+x)*scale,10+(row*10+y)*scale,scale,scale);
   ++column;
  }
  ++row;
 }
 glEnd();glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopAttrib();
}
}
