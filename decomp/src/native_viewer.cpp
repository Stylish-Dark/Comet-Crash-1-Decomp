#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include "comet/dds_image.hpp"
#include "gpu_render_targets.hpp"
#include "comet/native_deployment_input.hpp"
#include "comet/native_mission_playback.hpp"
#include "native_status_overlay.hpp"
#include "native_sdl_gamepad.hpp"
#include "comet/native_pilot.hpp"
#include "comet/native_mouse_input.hpp"
#include "native_mouse_toolbar.hpp"
#include <memory>
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <unordered_map>
namespace comet::decomp {
namespace {
struct Platform {
    SDL_Window* window=nullptr;SDL_GLContext context=nullptr;
    ~Platform(){if(context)SDL_GL_DeleteContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();}
};
struct BufferApi {
    PFNGLGENBUFFERSPROC gen;
    PFNGLDELETEBUFFERSPROC remove;
    PFNGLBINDBUFFERPROC bind;
    PFNGLBUFFERDATAPROC data;
    BufferApi() : gen(reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers"))),
        remove(reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers"))),
        bind(reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer"))),
        data(reinterpret_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData"))) {
        if(!gen||!remove||!bind||!data)throw std::runtime_error("OpenGL vertex buffers unavailable");
    }
};
struct GpuModels {
    BufferApi api;std::vector<std::array<GLuint,2>> buffers;
    std::unordered_map<std::string,GLuint> textures;
    ~GpuModels(){for(auto&b:buffers)api.remove(2,b.data());for(auto&[path,t]:textures)glDeleteTextures(1,&t);}
    void upload(const std::vector<ArenaNativeModel>& models){
        buffers.resize(models.size());
        for(std::size_t i=0;i<models.size();++i){
            const auto&m=models[i].model;auto&b=buffers[i];api.gen(2,b.data());
            api.bind(GL_ARRAY_BUFFER,b[0]);api.data(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(m.vertices.size()*sizeof(NativeVertex)),m.vertices.data(),GL_STATIC_DRAW);
            api.bind(GL_ELEMENT_ARRAY_BUFFER,b[1]);api.data(GL_ELEMENT_ARRAY_BUFFER,static_cast<GLsizeiptr>(m.indices.size()*sizeof(std::uint16_t)),m.indices.data(),GL_STATIC_DRAW);
        }
        for(const auto&entry:models)for(const auto&submesh:entry.model.submeshes){
            const auto&path=submesh.material.diffuse_texture;if(path.empty()||textures.contains(path.string()))continue;
            DdsImage image;const auto result=load_dds_image(path,image);if(!result)throw std::runtime_error(result.detail);
            GLuint texture=0;glGenTextures(1,&texture);textures.emplace(path.string(),texture);
            glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
            glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,image.rgba.data());
        }
        if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL mesh/texture upload failed");
    }
};
bool screenshot(const std::filesystem::path& path,int width,int height){
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    if(glGetError()!=GL_NO_ERROR)return false;
    std::ofstream output(path,std::ios::binary);output<<"P6\n"<<width<<" "<<height<<"\n255\n";
    const auto stride=static_cast<std::size_t>(width)*3;
    for(int y=height-1;y>=0;--y)output.write(reinterpret_cast<const char*>(pixels.data()+static_cast<std::size_t>(y)*stride),stride);
    return bool(output);
}
AssetLoadResult error(std::string detail){return {false,0,std::move(detail)};}
}
AssetLoadResult run_native_viewer(const std::vector<ArenaNativeModel>& models,const NativeViewerConfig& config){
    if(models.empty())return error("no models to render");
    if(config.width<1||config.height<1||config.width>8192||config.height>8192||config.initial_model>=models.size())return error("invalid viewer dimensions or model index");
    for(const auto&m:models){
        if(m.model.vertices.empty()||m.model.indices.empty())return error("empty model geometry");
        for(auto i:m.model.indices)if(i>=m.model.vertices.size())return error("invalid model index");
        for(const auto&s:m.model.submeshes)if(s.range.first_index>m.model.indices.size()||s.range.index_count>m.model.indices.size()-s.range.first_index||s.range.index_count>static_cast<unsigned>(std::numeric_limits<GLsizei>::max()))return error("invalid submesh draw range");
    }
    const ArenaState *arena=config.construction?&config.construction->arena():config.arena;
    Platform platform;SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMECONTROLLER)!=0)return error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    platform.window=SDL_CreateWindow("Comet Crash native model viewer",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,config.width,config.height,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|(config.hidden?SDL_WINDOW_HIDDEN:SDL_WINDOW_SHOWN));
    if(!platform.window)return error(SDL_GetError());
    platform.context=SDL_GL_CreateContext(platform.window);if(!platform.context)return error(SDL_GetError());
    SDL_GL_SetSwapInterval(1);
    try {
        GpuModels gpu;gpu.upload(models);GpuArenaTargets targets;
        std::cout<<"Native OpenGL: "<<glGetString(GL_RENDERER)<<" / "<<glGetString(GL_VERSION)<<'\n';
        glEnable(GL_DEPTH_TEST);glEnable(GL_NORMALIZE);glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);
        glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_NORMAL_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glMatrixMode(GL_TEXTURE);glLoadIdentity();glTranslatef(0,1,0);glScalef(1,-1,1);glMatrixMode(GL_MODELVIEW);
        glClearColor(0.025f,0.035f,0.055f,1);
        const GLfloat ambient[]={0.28f,0.28f,0.32f,1};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
        std::size_t selected=config.initial_model;float yaw=25,pitch=55,zoom=1;bool quit=false,wireframe=false;
        std::uint32_t frames=0;int width=config.width,height=config.height;
        ArenaGridPosition cursor{};std::optional<ArenaGridPosition> selected_factory;unsigned owner=0,opcode=20;bool manual_tick=false,focused=true;
        std::unique_ptr<NativeDeploymentInput> deployment;
        std::unique_ptr<NativeMissionPlayback> mission;
        if(config.construction){mission=std::make_unique<NativeMissionPlayback>(*config.construction);deployment=std::make_unique<NativeDeploymentInput>(*config.construction);owner=static_cast<unsigned>(deployment->owner());cursor=arena->players[owner].start_grid.value_or(ArenaGridPosition{});}
        detail::SdlGamepad gamepad;NativeGamepadInput pad_input;
        NativePilot pilot;bool pilot_mode=false;unsigned pilot_keys=0;
        const bool has_pilot_model=std::any_of(models.begin(),models.end(),[](const auto& m){return m.spec.original_slot_index==0;});
        if(deployment)pilot.reset(cursor,arena->extent);
        const auto pilot_key=[](SDL_Keycode key)->unsigned{return key==SDLK_i?1:key==SDLK_k?2:key==SDLK_j?4:key==SDLK_l?8:0;};
        bool keyboard_release=false,pad_release=false,mouse_release=false,last_build_queued=false;
        int mouse_x=-1,mouse_y=-1;
        auto cancel_input=[&]{keyboard_release=pad_release=mouse_release=false;SDL_CaptureMouse(SDL_FALSE);pilot_keys=0;pilot.stop();pad_input.cancel();if(deployment)deployment->cancel();};
        auto sync_release=[&]{if(deployment){if(!mission->finished()&&(keyboard_release||pad_release||mouse_release))deployment->press();else deployment->release();}};
        auto last_counter=SDL_GetPerformanceCounter();
        auto title=[&]{if(config.construction){auto text=std::string("Comet Crash construction | click: build/select, right drag: camera, toolbar: structures / release / play | cell ")+std::to_string(cursor.x)+","+std::to_string(cursor.z)+" owner "+std::to_string(owner)+" type "+std::to_string(opcode)+" resource "+std::to_string(config.construction->simulation().accounts()[owner].resource)+(pilot_mode?" PILOT PREVIEW":" GRID")+(mission->playing()?" playing":mission->finished()?" finished - F2 restart":" paused")+(config.construction->simulation().ground_transport_enabled()?" transport on":" transport off")+(config.construction->simulation().ground_dynamics_enabled()?" dynamics on":" dynamics off")+(config.construction->simulation().ground_contacts_enabled()?" contacts on":" contacts off");const auto* cell=arena->cell(cursor.x,cursor.z);if(cell&&cell->entity_index){const auto& entity=arena->entities[*cell->entity_index];if(entity.construction_progress)text+=entity.construction_ready?" ready":" building "+std::to_string(static_cast<int>(std::clamp(*entity.construction_progress,0.0f,1.0f)*100))+"%";if(entity.production_mode)text+=" factory mode "+std::to_string(*entity.production_mode)+" timer "+std::to_string(*entity.production_elapsed)+"/"+std::to_string(*entity.production_period)+" pending units "+std::to_string(config.construction->simulation().unit_quotas()[entity.owner].pending);}SDL_SetWindowTitle(platform.window,text.c_str());return;}if(arena){SDL_SetWindowTitle(platform.window,"Comet Crash recovered arena | drag: orbit, wheel: zoom, W: wireframe");return;}const auto&m=models[selected];auto s="Comet Crash native viewer | "+std::string(m.spec.path)+" | "+std::to_string(m.model.indices.size()/3)+" triangles | arrows: model, drag: orbit, wheel: zoom, W: wireframe";SDL_SetWindowTitle(platform.window,s.c_str());};title();
        auto command=[&](SDL_Keycode key){

            if(key==SDLK_F2){
                cancel_input();selected_factory.reset();deployment.reset();
                mission->restart();
                deployment=std::make_unique<NativeDeploymentInput>(*config.construction);
                owner=static_cast<unsigned>(deployment->owner());cursor=arena->players[owner].start_grid.value_or(ArenaGridPosition{});opcode=20;manual_tick=false;pilot.reset(cursor,arena->extent);
                last_counter=SDL_GetPerformanceCounter();title();return;
            }
            if(key==SDLK_F3){
                if(deployment->controllable()&&has_pilot_model&&!mission->finished()){cancel_input();pilot_mode=!pilot_mode;pilot.reset(cursor,arena->extent);}
                title();return;
            }
            if(pilot_key(key))pilot_keys|=pilot_key(key);
            if(pilot_mode&&(key==SDLK_RIGHT||key==SDLK_LEFT||key==SDLK_UP||key==SDLK_DOWN))return;
            if(key==SDLK_RIGHT&&cursor.x+1u<arena->extent)++cursor.x;
            if(key==SDLK_LEFT&&cursor.x)--cursor.x;
            if(key==SDLK_DOWN&&cursor.z+1u<arena->extent)++cursor.z;
            if(key==SDLK_UP&&cursor.z)--cursor.z;
            if(key>=SDLK_1&&key<=SDLK_8){constexpr unsigned types[]={20,21,22,23,24,25,26,28};opcode=types[key-SDLK_1];}
            if(key==SDLK_TAB){cancel_input();selected_factory.reset();deployment->next_owner();owner=static_cast<unsigned>(deployment->owner());cursor=arena->players[owner].start_grid.value_or(ArenaGridPosition{});pilot.reset(cursor,arena->extent);}
            if(key==SDLK_q)deployment->cycle_kind(-1);
            if(key==SDLK_e)deployment->cycle_kind(1);
            // Inspection/camera remain available; finished matches reject mutations.
            if(mission->finished() && key!=SDLK_w && key!=SDLK_r){title();return;}
            if(key==SDLK_d){keyboard_release=true;sync_release();}
            if(deployment->controllable()&&(key==SDLK_b||key==SDLK_SPACE)){auto result=config.construction->enqueue({static_cast<std::uint8_t>(owner),static_cast<std::uint8_t>(opcode),cursor.x,cursor.z});std::cout<<"Build submission: "<<static_cast<unsigned>(result)<<'\n';last_build_queued=result==ArenaConstructionSubmit::Queued;if(last_build_queued)manual_tick=true;}
            if(deployment->controllable()&&key>=SDLK_F5&&key<=SDLK_F9){auto result=config.construction->select_factory(owner,cursor,static_cast<std::uint8_t>(4+key-SDLK_F5));std::cout<<"Factory selection: "<<static_cast<unsigned>(result)<<'\n';}
            if(deployment->controllable()&&key==SDLK_u){
                const auto &quota=config.construction->simulation().unit_quotas()[owner];
                for(unsigned kind=40;kind<=47;++kind){if(kind==42||!quota.by_type[kind-40])continue;const auto result=config.construction->spawn_reserved_ground_unit(static_cast<std::uint8_t>(owner),static_cast<std::uint8_t>(kind),cursor);std::cout<<"Unit spawn: "<<static_cast<unsigned>(result.status)<<'\n';break;}
            }
            if(key==SDLK_c){config.construction->set_ground_contacts(!config.construction->simulation().ground_contacts_enabled());std::cout<<"Ground motion contacts: "<<config.construction->simulation().ground_contacts_enabled()<<'\n';title();}
            if(key==SDLK_n){config.construction->set_ground_dynamics(!config.construction->simulation().ground_dynamics_enabled());std::cout<<"Original ground velocity dynamics: "<<config.construction->simulation().ground_dynamics_enabled()<<'\n';title();}
            if(key==SDLK_m){config.construction->set_ground_transport(!config.construction->simulation().ground_transport_enabled());std::cout<<"Native ground transport: "<<config.construction->simulation().ground_transport_enabled()<<'\n';title();}
            if(key==SDLK_t)manual_tick=true;
            if(key==SDLK_p){mission->toggle_playing();cancel_input();}
            title();
        };
        auto status_lines=[&]{
            auto lines=deployment->status_lines(mission->playing(),gamepad.sample().connected);
            if(selected_factory)lines[3]="FACTORY "+std::to_string(selected_factory->x)+","+std::to_string(selected_factory->z)+" SELECTED  FACTORY MODE CHANGES ITS PRODUCTION";
            lines[4]=pilot_mode?"SHIP: IJKL/STICK MOVE  F3 GRID":"LEFT CLICK BUILD/SELECT  RIGHT DRAG CAMERA  WHEEL ZOOM";
            return lines;
        };
        auto mouse_cell=[&](int x,int y)->bool{
            if(!deployment||pilot_mode||!focused)return false;
            int ww,wh;SDL_GetWindowSize(platform.window,&ww,&wh);
            if(wh<=0||native_mouse_toolbar_at(y,wh)||double(y)*height/wh<detail::status_overlay_height(status_lines(),width))return false;
            const auto hit=native_mouse_cell(x,y,ww,wh,arena->extent,yaw,pitch,zoom);
            if(!hit)return false;
            cursor=*hit;title();return true;
        };
        auto mouse_command=[&](int button){
            if(button==9){command(SDLK_F2);return;}
            if(mission->finished())return;
            if(button==8){command(SDLK_p);return;}
            if(!deployment->controllable())return;
            if(button>=0&&button<8){opcode=detail::mouse_build_types[button];title();}
            if(button==10){mouse_release=true;SDL_CaptureMouse(SDL_TRUE);sync_release();title();}
            if(button==11)command(SDLK_q);
            if(button==12)command(SDLK_e);
            if(button==13&&selected_factory){
                const auto* cell=arena->cell(selected_factory->x,selected_factory->z);
                if(cell&&cell->entity_index){const auto& entity=arena->entities[*cell->entity_index];
                    if(entity.owner==owner&&(entity.type==26||entity.type==28)&&entity.production_mode)
                        config.construction->select_factory(owner,*selected_factory,static_cast<std::uint8_t>(4+(*entity.production_mode+1)%5));
                }
                title();
            }
            if(button==14)command(SDLK_F3);
        };
        while(!quit){
            SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_QUIT)quit=true;
                if(gamepad.event(event) && deployment){cancel_input();mission->pause();manual_tick=false;title();}
                if(config.construction && event.type==SDL_WINDOWEVENT){
                    if(event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){cancel_input();mission->pause();manual_tick=false;focused=false;title();}
                    if(event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)focused=true;
                }
                if(event.type==SDL_KEYUP)pilot_keys&=~pilot_key(event.key.keysym.sym);
                if(deployment && event.type==SDL_KEYUP && event.key.keysym.sym==SDLK_d){keyboard_release=false;sync_release();title();}
                if(event.type==SDL_KEYDOWN && focused){
                    if(event.key.keysym.sym==SDLK_ESCAPE)quit=true;
                    if(!arena&&(event.key.keysym.sym==SDLK_RIGHT||event.key.keysym.sym==SDLK_LEFT)){selected=(selected+models.size()+(event.key.keysym.sym==SDLK_RIGHT?1:-1))%models.size();zoom=1;title();}
                    if(config.construction&&!event.key.repeat)command(event.key.keysym.sym);
                    if(event.key.keysym.sym==SDLK_w)wireframe=!wireframe;
                    if(event.key.keysym.sym==SDLK_r){yaw=25;pitch=55;zoom=1;}
                }
                if(event.type==SDL_MOUSEMOTION){
                    mouse_x=event.motion.x;mouse_y=event.motion.y;
                    const bool orbit=event.motion.state&(deployment?SDL_BUTTON_RMASK:SDL_BUTTON_LMASK);
                    if(orbit&&focused){yaw+=event.motion.xrel*0.5f;pitch=std::clamp(pitch+event.motion.yrel*0.5f,-89.0f,89.0f);}
                    else if(deployment)mouse_cell(mouse_x,mouse_y);
                }
                if(deployment&&event.type==SDL_MOUSEBUTTONDOWN&&event.button.button==SDL_BUTTON_LEFT&&focused){
                    mouse_x=event.button.x;mouse_y=event.button.y;
                    int ww,wh;SDL_GetWindowSize(platform.window,&ww,&wh);
                    const int button=native_mouse_button_at(mouse_x,mouse_y,ww,wh);
                    if(button>=0)mouse_command(button);
                    else if(!mission->finished()&&mouse_cell(mouse_x,mouse_y)){
                        const auto* cell=arena->cell(cursor.x,cursor.z);
                        // Clicking an existing structure selects it; it never
                        // submits a second building on the occupied cell.
                        selected_factory.reset();
                        if(cell&&cell->entity_index){const auto& entity=arena->entities[*cell->entity_index];
                            if(entity.owner==owner&&(entity.type==26||entity.type==28))selected_factory=cursor;
                        }
                        if(cell&&!cell->entity_index){last_build_queued=false;command(SDLK_b);if(last_build_queued&&(opcode==26||opcode==28))selected_factory=cursor;}
                    }
                }
                if(deployment&&event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT){
                    mouse_release=false;SDL_CaptureMouse(SDL_FALSE);sync_release();title();
                }
                if(event.type==SDL_MOUSEWHEEL)zoom=std::clamp(zoom*std::pow(0.9f,static_cast<float>(event.wheel.y)),0.1f,20.0f);
            }
            if(quit)break;
            auto counter=SDL_GetPerformanceCounter();
            if(deployment){
                const float elapsed=std::clamp(static_cast<float>(double(counter-last_counter)/SDL_GetPerformanceFrequency()),0.0f,.25f);
                auto sample=gamepad.sample();
                const auto a=pad_input.advance(sample,elapsed,focused);
                if(a.restart)command(SDLK_F2);
                else if(a.next_owner)command(SDLK_TAB);
                else if(a.cancel){cancel_input();title();}
                else {
                    if(pilot_mode){
                        float x=a.move_x,z=a.move_z;
                        if(pilot_keys){x=int(bool(pilot_keys&8))-int(bool(pilot_keys&4));z=int(bool(pilot_keys&2))-int(bool(pilot_keys&1));}
                        pilot.advance(x,z,elapsed,focused&&mission->playing()&&!mission->finished()&&deployment->controllable());
                        cursor=pilot.cursor();
                    }else {
                        if(a.dx)command(a.dx>0?SDLK_RIGHT:SDLK_LEFT);
                        if(a.dz)command(a.dz>0?SDLK_DOWN:SDLK_UP);
                    }
                    if(a.build_delta){
                        constexpr unsigned types[]={20,21,22,23,24,25,26,28};
                        const auto i=std::find(std::begin(types),std::end(types),opcode)-std::begin(types);
                        opcode=types[(i+8+a.build_delta)%8];title();
                    }
                    if(a.next_kind)command(SDLK_e);
                    if(a.factory){
                        unsigned mode=0;const auto* cell=arena->cell(cursor.x,cursor.z);
                        if(cell && cell->entity_index){const auto& e=arena->entities[*cell->entity_index];if(e.production_mode)mode=(*e.production_mode+1)%5;}
                        command(SDLK_F5+mode);
                    }
                    if(a.build)command(SDLK_b);
                    if(a.pause)command(SDLK_p);
                    else {pad_release=a.release_held;sync_release();}
                }
            }
            if(config.construction&&(mission->playing()||manual_tick)){
                const auto now=SDL_GetPerformanceCounter();
                const auto delta=mission->playing()?static_cast<float>(double(now-last_counter)/SDL_GetPerformanceFrequency()):1.0f;
                mission->advance(delta);manual_tick=false;
                if(mission->finished())cancel_input();
                title();
            }
            last_counter=counter;
            SDL_GL_GetDrawableSize(platform.window,&width,&height);
            if(width<=0||height<=0){SDL_Delay(16);continue;}
            const auto&m=models[selected].model;
            const ModelPosition center=arena?ModelPosition{arena->extent*0.5f,0,arena->extent*0.5f}:ModelPosition{(m.bounds_min.x+m.bounds_max.x)*0.5f,(m.bounds_min.y+m.bounds_max.y)*0.5f,(m.bounds_min.z+m.bounds_max.z)*0.5f};
            const auto radius=arena?arena->extent*0.6f:std::max(0.01f,0.5f*std::hypot(m.bounds_max.x-m.bounds_min.x,m.bounds_max.y-m.bounds_min.y,m.bounds_max.z-m.bounds_min.z));
            if(!targets.matches(width,height)){
                targets.resize(width,height);
                std::cout<<"Native arena resources: "<<targets.texture_count()<<" textures, "<<targets.framebuffer_count()<<" complete framebuffers\n";
            }
            targets.begin();glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glMatrixMode(GL_PROJECTION);glLoadIdentity();const double near_plane=std::max(0.0001,double(radius)*0.01);
            const double aspect=double(width)/height;const double top=near_plane*std::tan(22.5*3.141592653589793/180);
            glFrustum(-top*aspect,top*aspect,-top,top,near_plane,double(radius)*200*zoom);
            glMatrixMode(GL_MODELVIEW);glLoadIdentity();
            const GLfloat light[]={-3,5,8,0};glLightfv(GL_LIGHT0,GL_POSITION,light);
            glTranslatef(0,0,-radius*3.2f*zoom/std::min(1.0f,static_cast<float>(aspect)));
            glRotatef(pitch,1,0,0);glRotatef(yaw,0,1,0);glTranslatef(-center.x,-center.y,-center.z);
            auto draw=[&](std::size_t index){
            gpu.api.bind(GL_ARRAY_BUFFER,gpu.buffers[index][0]);gpu.api.bind(GL_ELEMENT_ARRAY_BUFFER,gpu.buffers[index][1]);
            glVertexPointer(3,GL_FLOAT,sizeof(NativeVertex),reinterpret_cast<const void*>(offsetof(NativeVertex,position)));
            glNormalPointer(GL_FLOAT,sizeof(NativeVertex),reinterpret_cast<const void*>(offsetof(NativeVertex,normal)));
            glTexCoordPointer(2,GL_FLOAT,sizeof(NativeVertex),reinterpret_cast<const void*>(offsetof(NativeVertex,u)));
            glPolygonMode(GL_FRONT_AND_BACK,wireframe?GL_LINE:GL_FILL);
            for(const auto&submesh:models[index].model.submeshes){
                const auto&c=submesh.material.properties.diffuse;
                const bool textured=!submesh.material.diffuse_texture.empty();
                const GLfloat diffuse[]={textured?1.0f:c.r,textured?1.0f:c.g,textured?1.0f:c.b,1};glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,diffuse);
                if(textured){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,gpu.textures.at(submesh.material.diffuse_texture.string()));}else glDisable(GL_TEXTURE_2D);
                glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(submesh.range.index_count),GL_UNSIGNED_SHORT,reinterpret_cast<const void*>(static_cast<std::uintptr_t>(submesh.range.first_index)*sizeof(std::uint16_t)));
            }
            };
            if(arena){
                auto instance=[&](std::uint32_t slot,ModelPosition position,const std::array<float,4>& orientation,float scale){
                    const auto found=std::find_if(models.begin(),models.end(),[&](const auto& model){return model.spec.original_slot_index==slot;});
                    if(found==models.end())throw std::runtime_error("arena entity model slot missing: "+std::to_string(slot));
                    glPushMatrix();glTranslatef(position.x,position.y,position.z);
                    const auto& q=orientation;
                    // Standard x/y/z/w quaternion display transform; simulation retains raw words.
                    const GLfloat rotation[]={1-2*(q[1]*q[1]+q[2]*q[2]),2*(q[0]*q[1]+q[2]*q[3]),2*(q[0]*q[2]-q[1]*q[3]),0,
                        2*(q[0]*q[1]-q[2]*q[3]),1-2*(q[0]*q[0]+q[2]*q[2]),2*(q[1]*q[2]+q[0]*q[3]),0,
                        2*(q[0]*q[2]+q[1]*q[3]),2*(q[1]*q[2]-q[0]*q[3]),1-2*(q[0]*q[0]+q[1]*q[1]),0,0,0,0,1};
                    glMultMatrixf(rotation);glScalef(scale,scale,scale);draw(static_cast<std::size_t>(found-models.begin()));glPopMatrix();
                };
                if(pilot_mode&&deployment->controllable())instance(0,pilot.position(),pilot.orientation(),1.f);
                instance(37,arena->background_position,arena->background_orientation,1.f);
                for(const auto& entity:arena->entities)
                    if(!entity.beam_path)instance(entity.model_slot,entity.position,entity.orientation,entity.corpse_fraction.value_or(1.f));
                // Native preview lines follow recovered beam control points;
                // the original ribbon shader/material remains unrecovered.
                glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LINE_BIT);
                glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glColor3f(.35f,.75f,1.f);glLineWidth(2.f);
                for(const auto& entity:arena->entities)if(entity.beam_path){
                    glBegin(GL_LINE_STRIP);
                    for(const auto& point:*entity.beam_path)glVertex3f(point.x,point.y,point.z);
                    glEnd();
                }
                glPopAttrib();
                if(config.construction){
                    gpu.api.bind(GL_ARRAY_BUFFER,0);gpu.api.bind(GL_ELEMENT_ARRAY_BUFFER,0);
                    glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glDisableClientState(GL_VERTEX_ARRAY);glDisableClientState(GL_NORMAL_ARRAY);glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                    for(const auto& entity:arena->entities){
                        if(!entity.construction_progress||entity.construction_ready)continue;
                        const float progress=std::clamp(*entity.construction_progress,0.0f,1.0f);
                        glColor3f(0.1f,0.9f,1.0f);glBegin(GL_LINES);
                        glVertex3f(entity.grid.x+0.1f,0.08f,entity.grid.z+0.1f);
                        glVertex3f(entity.grid.x+0.1f+0.8f*progress,0.08f,entity.grid.z+0.1f);glEnd();
                    }
                    if(selected_factory){
                        const auto p=*selected_factory;glColor3f(.1f,.6f,1.f);glLineWidth(3.f);
                        glBegin(GL_LINE_LOOP);glVertex3f(p.x,.06f,p.z);glVertex3f(p.x+1,.06f,p.z);glVertex3f(p.x+1,.06f,p.z+1);glVertex3f(p.x,.06f,p.z+1);glEnd();glLineWidth(1.f);
                    }
                    glColor3f(1,1,0);glBegin(GL_LINE_LOOP);glVertex3f(cursor.x,0.05f,cursor.z);glVertex3f(cursor.x+1,0.05f,cursor.z);glVertex3f(cursor.x+1,0.05f,cursor.z+1);glVertex3f(cursor.x,0.05f,cursor.z+1);glEnd();
                    glEnable(GL_LIGHTING);glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_NORMAL_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                }
            }else draw(selected);
            if(glGetError()!=GL_NO_ERROR)return error("OpenGL draw failed");
            targets.present();
            if(deployment){
                detail::draw_status_overlay(status_lines(),width,height);
                int ww,wh;SDL_GetWindowSize(platform.window,&ww,&wh);
                detail::draw_mouse_toolbar(ww,wh,width,height,opcode,mission->playing(),mission->finished(),deployment->controllable(),
                    keyboard_release||pad_release||mouse_release,pilot_mode,native_mouse_button_at(mouse_x,mouse_y,ww,wh));
            }
            if(glGetError()!=GL_NO_ERROR)return error("OpenGL status overlay failed");
            ++frames;
            if(!config.screenshot.empty() && (config.frame_limit==0||frames==config.frame_limit)){
                if(!screenshot(config.screenshot,width,height))return error("could not save rendered screenshot");
                if(config.frame_limit==0)quit=true;
            }
            SDL_GL_SwapWindow(platform.window);
            if(config.frame_limit&&frames>=config.frame_limit)quit=true;
            SDL_Delay(1);
        }
        std::cout<<"Native viewer rendered "<<frames<<" frames\n";
    }catch(const std::exception&ex){return error(ex.what());}
    return {};
}
} // namespace comet::decomp
