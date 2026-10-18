#pragma once
#include "comet/native_gamepad_input.hpp"
#include <SDL.h>
namespace comet::decomp::detail {
// Own one SDL-standardized controller. Additional devices wait for its removal.
class SdlGamepad {
 SDL_GameController* handle_=nullptr;
 void discover(){
  if(handle_)return;
  for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_IsGameController(i)){
   handle_=SDL_GameControllerOpen(i);if(handle_)break;
  }
 }
public:
 SdlGamepad(){discover();}
 ~SdlGamepad(){if(handle_)SDL_GameControllerClose(handle_);}
 SdlGamepad(const SdlGamepad&)=delete;
 SdlGamepad& operator=(const SdlGamepad&)=delete;
 // True only when the active device disconnects; caller cancels held actions.
 bool event(const SDL_Event& e){
  if(e.type==SDL_CONTROLLERDEVICEADDED)discover();
  if(e.type!=SDL_CONTROLLERDEVICEREMOVED || !handle_ ||
     e.cdevice.which!=SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(handle_)))return false;
  SDL_GameControllerClose(handle_);handle_=nullptr;discover();return true;
 }
 NativeGamepadSample sample() const {
  NativeGamepadSample s;
  if(!handle_ || !SDL_GameControllerGetAttached(handle_))return s;
  s.connected=true;s.x=SDL_GameControllerGetAxis(handle_,SDL_CONTROLLER_AXIS_LEFTX);
  s.z=SDL_GameControllerGetAxis(handle_,SDL_CONTROLLER_AXIS_LEFTY);
  const auto button=[&](SDL_GameControllerButton b,NativePadButton action){if(SDL_GameControllerGetButton(handle_,b))s.buttons|=action;};
  button(SDL_CONTROLLER_BUTTON_A,PadBuild);button(SDL_CONTROLLER_BUTTON_B,PadCancel);
  button(SDL_CONTROLLER_BUTTON_X,PadRelease);button(SDL_CONTROLLER_BUTTON_Y,PadFactory);
  button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,PadPreviousBuild);button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,PadNextBuild);
  button(SDL_CONTROLLER_BUTTON_LEFTSTICK,PadNextKind);button(SDL_CONTROLLER_BUTTON_RIGHTSTICK,PadNextOwner);
  button(SDL_CONTROLLER_BUTTON_START,PadPause);button(SDL_CONTROLLER_BUTTON_BACK,PadRestart);
  button(SDL_CONTROLLER_BUTTON_DPAD_UP,PadUp);button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,PadDown);
  button(SDL_CONTROLLER_BUTTON_DPAD_LEFT,PadLeft);button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,PadRight);
  if(SDL_GameControllerGetAxis(handle_,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>3276)s.buttons|=PadRelease;
  return s;
 }
};
}
