#pragma once
#include "comet/arena_render_targets.hpp"
#include <SDL.h>
#include <SDL_opengl.h>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <vector>
namespace comet::decomp {
// Native resource owners. Legacy offsets are lookup keys, never memory addresses.
class GpuArenaTargets {
    PFNGLGENFRAMEBUFFERSPROC gen_=reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glGenFramebuffers"));
    PFNGLDELETEFRAMEBUFFERSPROC delete_=reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteFramebuffers"));
    PFNGLBINDFRAMEBUFFERPROC bind_=reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBindFramebuffer"));
    PFNGLFRAMEBUFFERTEXTURE2DPROC attach_=reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(SDL_GL_GetProcAddress("glFramebufferTexture2D"));
    PFNGLCHECKFRAMEBUFFERSTATUSPROC status_=reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(SDL_GL_GetProcAddress("glCheckFramebufferStatus"));
    PFNGLDRAWBUFFERSPROC draw_=reinterpret_cast<PFNGLDRAWBUFFERSPROC>(SDL_GL_GetProcAddress("glDrawBuffers"));
    PFNGLBLITFRAMEBUFFERPROC blit_=reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBlitFramebuffer"));
    std::map<std::uint32_t,GLuint> textures_,framebuffers_;
    int width_=0,height_=0;
    void clear(){
        for(auto&[key,f]:framebuffers_)delete_(1,&f);
        for(auto&[key,t]:textures_)glDeleteTextures(1,&t);
        framebuffers_.clear();textures_.clear();
    }
public:
    GpuArenaTargets(){if(!gen_||!delete_||!bind_||!attach_||!status_||!draw_||!blit_)throw std::runtime_error("OpenGL framebuffer API unavailable");}
    ~GpuArenaTargets(){clear();}
    GpuArenaTargets(const GpuArenaTargets&)=delete;
    GpuArenaTargets& operator=(const GpuArenaTargets&)=delete;
    bool matches(int width,int height)const{return width_==width&&height_==height;}
    std::size_t texture_count()const{return textures_.size();}
    std::size_t framebuffer_count()const{return framebuffers_.size();}
    void resize(int width,int height){
        clear();width_=0;height_=0;
        const auto plan=recover_arena_render_target_plan(width,height,std::max(1,width/2),std::max(1,height/2),0);
        GLint max_size=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);
        for(const auto&spec:plan.textures){
            if(!spec.width||!spec.height||spec.width>static_cast<unsigned>(max_size)||spec.height>static_cast<unsigned>(max_size))throw std::runtime_error("arena render target exceeds GPU limits");
            GLuint texture=0;glGenTextures(1,&texture);textures_.emplace(spec.original_texture_offset,texture);glBindTexture(GL_TEXTURE_2D,texture);
            const auto filter=[](TextureFilter f){return f==TextureFilter::Linear?GL_LINEAR:GL_NEAREST;};
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter(spec.min_filter));glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter(spec.mag_filter));
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            GLint storage=GL_RGBA8;GLenum format=GL_RGBA,type=GL_UNSIGNED_BYTE;
            if(spec.storage==TextureStorage::Rgb16Float){storage=GL_RGB16F;format=GL_RGB;type=GL_FLOAT;}
            if(spec.storage==TextureStorage::Rgba16Float){storage=GL_RGBA16F;type=GL_FLOAT;}
            if(spec.storage==TextureStorage::Depth32){storage=GL_DEPTH_COMPONENT32F;format=GL_DEPTH_COMPONENT;type=GL_FLOAT;}
            glTexImage2D(GL_TEXTURE_2D,0,storage,spec.width,spec.height,0,format,type,nullptr);
        }
        std::map<std::uint32_t,std::vector<GLenum>> colors;
        for(const auto&spec:plan.attachments){
            auto&f=framebuffers_[spec.original_framebuffer_offset];if(!f)gen_(1,&f);bind_(GL_FRAMEBUFFER,f);
            GLenum attachment=spec.attachment==TargetAttachment::Depth?GL_DEPTH_ATTACHMENT:spec.attachment==TargetAttachment::Color1?GL_COLOR_ATTACHMENT1:GL_COLOR_ATTACHMENT0;
            attach_(GL_FRAMEBUFFER,attachment,GL_TEXTURE_2D,textures_.at(spec.original_texture_offset),0);
            if(attachment!=GL_DEPTH_ATTACHMENT){auto&list=colors[spec.original_framebuffer_offset];if(std::find(list.begin(),list.end(),attachment)==list.end())list.push_back(attachment);}
        }
        for(auto&[key,f]:framebuffers_){bind_(GL_FRAMEBUFFER,f);auto&list=colors[key];draw_(static_cast<GLsizei>(list.size()),list.data());if(status_(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("native arena framebuffer incomplete: "+std::to_string(key));}
        bind_(GL_FRAMEBUFFER,0);if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("native arena target allocation failed");
        width_=width;height_=height;
    }
    void begin(){bind_(GL_FRAMEBUFFER,framebuffers_.at(0x445C));glViewport(0,0,width_*2,height_*2);}
    void present(){
        bind_(GL_READ_FRAMEBUFFER,framebuffers_.at(0x445C));bind_(GL_DRAW_FRAMEBUFFER,0);
        blit_(0,0,width_*2,height_*2,0,0,width_,height_,GL_COLOR_BUFFER_BIT,GL_LINEAR);bind_(GL_FRAMEBUFFER,0);
    }
};
}
