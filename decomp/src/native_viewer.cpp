#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include "comet/dds_image.hpp"
#include "gpu_render_targets.hpp"
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
    Platform platform;SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)!=0)return error(SDL_GetError());
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
        auto title=[&]{if(config.arena){SDL_SetWindowTitle(platform.window,"Comet Crash recovered arena | drag: orbit, wheel: zoom, W: wireframe");return;}const auto&m=models[selected];auto s="Comet Crash native viewer | "+std::string(m.spec.path)+" | "+std::to_string(m.model.indices.size()/3)+" triangles | arrows: model, drag: orbit, wheel: zoom, W: wireframe";SDL_SetWindowTitle(platform.window,s.c_str());};title();
        while(!quit){
            SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_QUIT)quit=true;
                if(event.type==SDL_KEYDOWN){
                    if(event.key.keysym.sym==SDLK_ESCAPE)quit=true;
                    if(!config.arena&&(event.key.keysym.sym==SDLK_RIGHT||event.key.keysym.sym==SDLK_LEFT)){selected=(selected+models.size()+(event.key.keysym.sym==SDLK_RIGHT?1:-1))%models.size();zoom=1;title();}
                    if(event.key.keysym.sym==SDLK_w)wireframe=!wireframe;
                    if(event.key.keysym.sym==SDLK_r){yaw=25;pitch=55;zoom=1;}
                }
                if(event.type==SDL_MOUSEMOTION&&(event.motion.state&SDL_BUTTON_LMASK)){yaw+=event.motion.xrel*0.5f;pitch=std::clamp(pitch+event.motion.yrel*0.5f,-89.0f,89.0f);}
                if(event.type==SDL_MOUSEWHEEL)zoom=std::clamp(zoom*std::pow(0.9f,static_cast<float>(event.wheel.y)),0.1f,20.0f);
            }
            if(quit)break;
            SDL_GL_GetDrawableSize(platform.window,&width,&height);
            if(width<=0||height<=0){SDL_Delay(16);continue;}
            const auto&m=models[selected].model;
            const ModelPosition center=config.arena?ModelPosition{config.arena->extent*0.5f,0,config.arena->extent*0.5f}:ModelPosition{(m.bounds_min.x+m.bounds_max.x)*0.5f,(m.bounds_min.y+m.bounds_max.y)*0.5f,(m.bounds_min.z+m.bounds_max.z)*0.5f};
            const auto radius=config.arena?config.arena->extent*0.6f:std::max(0.01f,0.5f*std::hypot(m.bounds_max.x-m.bounds_min.x,m.bounds_max.y-m.bounds_min.y,m.bounds_max.z-m.bounds_min.z));
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
            if(config.arena){
                auto instance=[&](std::uint32_t slot,ModelPosition position,const std::array<float,4>& orientation){
                    const auto found=std::find_if(models.begin(),models.end(),[&](const auto& model){return model.spec.original_slot_index==slot;});
                    if(found==models.end())throw std::runtime_error("arena entity model slot missing: "+std::to_string(slot));
                    glPushMatrix();glTranslatef(position.x,position.y,position.z);
                    const auto& q=orientation;
                    // Standard x/y/z/w quaternion display transform; simulation retains raw words.
                    const GLfloat rotation[]={1-2*(q[1]*q[1]+q[2]*q[2]),2*(q[0]*q[1]+q[2]*q[3]),2*(q[0]*q[2]-q[1]*q[3]),0,
                        2*(q[0]*q[1]-q[2]*q[3]),1-2*(q[0]*q[0]+q[2]*q[2]),2*(q[1]*q[2]+q[0]*q[3]),0,
                        2*(q[0]*q[2]+q[1]*q[3]),2*(q[1]*q[2]-q[0]*q[3]),1-2*(q[0]*q[0]+q[1]*q[1]),0,0,0,0,1};
                    glMultMatrixf(rotation);draw(static_cast<std::size_t>(found-models.begin()));glPopMatrix();
                };
                instance(37,config.arena->background_position,config.arena->background_orientation);
                for(const auto& entity:config.arena->entities)instance(entity.model_slot,entity.position,entity.orientation);
            }else draw(selected);
            if(glGetError()!=GL_NO_ERROR)return error("OpenGL draw failed");
            targets.present();
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
