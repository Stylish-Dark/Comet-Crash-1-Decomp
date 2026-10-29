#pragma once
#include "comet/arena_state.hpp"
namespace comet::decomp {
// PC input adaptation for the existing arena camera. Coordinates are logical
// window pixels, matching SDL mouse events rather than drawable DPI pixels.
std::optional<ArenaGridPosition> native_mouse_cell(int x,int y,int width,int height,
    unsigned extent,float yaw,float pitch,float zoom);
struct NativeMouseRect {
 int x=0,y=0,w=0,h=0;
 bool contains(int px,int py) const {return px>=x&&py>=y&&px<x+w&&py<y+h;}
};
// Eight structure buttons followed by play, restart, hold release, previous
// unit, next unit, factory mode and pilot mode. Layout and hit testing agree.
NativeMouseRect native_mouse_button(unsigned index,int width,int height);
int native_mouse_button_at(int x,int y,int width,int height);
bool native_mouse_toolbar_at(int y,int height);
}
