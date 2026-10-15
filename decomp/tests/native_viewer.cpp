#define SDL_MAIN_HANDLED
#include "comet/native_viewer.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <array>
#include <thread>
#include <atomic>
#include <SDL.h>
using namespace comet::decomp;
int main() {
    NativeViewerConfig config;config.width=320;config.height=240;config.frame_limit=2;config.hidden=true;
    config.screenshot=std::filesystem::temp_directory_path()/"comet-render-test.ppm";
    ArenaNativeModel m{arena_model_asset_manifest()[0],{}};
    m.model.vertices={{{-1,-1,0},{0,0,1},0,0},{{1,-1,0},{0,0,1},1,0},{{0,1,0},{0,0,1},0.5f,1}};
    m.model.indices={0,1,2};NativeSubmesh mesh;mesh.range={0,3,0,2};mesh.material.properties.diffuse={0.9f,0.3f,0.1f,0};m.model.submeshes={mesh};m.model.bounds_min={-1,-1,0};m.model.bounds_max={1,1,0};
    // A green texture over a red material proves the graphics path samples DDS.
    auto texture=config.screenshot;texture.replace_extension(".dds");
    std::array<unsigned char,132> dds{};auto put=[&](int offset,unsigned value){for(int i=0;i<5;++i)dds[offset+i]=value>>(i*8);};
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
    ArenaState arena;arena.extent=4;arena.background_position={2,0,2};
    ArenaEntity entity;entity.model_slot=m.spec.original_slot_index;entity.position={1,0,1};arena.entities.push_back(entity);
    auto background=m;background.spec.original_slot_index=37;
    config.arena=&arena;
    const auto scene=run_native_viewer({m,background},config);
    if(!scene)throw std::runtime_error(scene.detail);
    const auto missing=run_native_viewer({m},config);
    if(missing||missing.detail.find("slot missing: 37")==std::string::npos)throw std::runtime_error("missing arena model was not rejected");
    ArenaConstructionSimulationConfig construction;construction.production_enabled=true;construction.unit_quotas[0].capacity=2;construction.capacity=4;construction.players={{{{0,0},0,false},400,400}};construction.tuning[26]=ArenaStructureTableValues{100,50,10,20,7.5f};
    ArenaState editable;editable.extent=4;editable.players.resize(1);editable.players[0].start_grid=ArenaGridPosition{0,0};editable.background_position={2,0,2};
    NativeConstructionScene editable_scene(editable,construction);auto structure=m;structure.spec.original_slot_index=*initial_model_slot_for_record(26);
    auto spawned_model=m;spawned_model.spec.original_slot_index=2;
    config.arena=nullptr;config.construction=&editable_scene;config.frame_limit=0;config.screenshot.clear();
    std::atomic<bool> finished=false;
    std::thread controls([&]{
        while(!SDL_WasInit(SDL_INIT_VIDEO)){if(finished.load())return;SDL_Delay(1);}
        SDL_Delay(100);SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.sym=SDLK_7;SDL_PushEvent(&key);key.key.keysym.sym=SDLK_b;SDL_PushEvent(&key);
        for(int i=0;i<5;++i){SDL_Delay(100);key.key.keysym.sym=SDLK_t;SDL_PushEvent(&key);}
        SDL_Delay(100);key.key.keysym.sym=SDLK_F9;SDL_PushEvent(&key);
        SDL_Delay(100);key.key.keysym.sym=SDLK_t;SDL_PushEvent(&key);
        SDL_Delay(100);key.key.keysym.sym=SDLK_RIGHT;SDL_PushEvent(&key);key.key.keysym.sym=SDLK_DOWN;SDL_PushEvent(&key);key.key.keysym.sym=SDLK_u;SDL_PushEvent(&key);
        SDL_Delay(100);SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
    });
    auto built=run_native_viewer({structure,spawned_model,background},config);finished.store(true);controls.join();
    if(!built)throw std::runtime_error(built.detail);
    if(editable_scene.arena().entities.size()!=2||editable_scene.simulation().accounts()[0].resource!=340)throw std::runtime_error("keyboard construction did not commit");
    if(!editable_scene.arena().entities[0].construction_ready)throw std::runtime_error("keyboard ticks did not finish construction");
    if(editable_scene.arena().entities[0].production_mode!=4)throw std::runtime_error("keyboard factory selection did not commit");
    if(editable_scene.simulation().unit_quotas()[0].pending!=0||editable_scene.arena().entities[0].production_period!=20.0f||editable_scene.arena().entities[0].production_elapsed!=1.0f)throw std::runtime_error("keyboard production did not reserve or refresh: clock="+std::to_string(editable_scene.simulation().clock())+" pending="+std::to_string(editable_scene.simulation().unit_quotas()[0].pending)+" period="+std::to_string(*editable_scene.arena().entities[0].production_period)+" elapsed="+std::to_string(*editable_scene.arena().entities[0].production_elapsed));
    if(editable_scene.simulation().unit_quotas()[0].active!=1||editable_scene.arena().entities[1].type!=40||editable_scene.arena().entities[1].grid!=ArenaGridPosition{1,1}||editable_scene.arena().entities[1].construction_progress)throw std::runtime_error("keyboard unit spawn did not project");
    const auto unit=editable_scene.simulation().factory_unit_template(*editable_scene.arena().entities[0].live_entity_index);
    if(!unit||(*unit)[15]!=44||(*unit)[0x6f]!=6)throw std::runtime_error("keyboard factory unit inspection failed");
    const auto ground=editable_scene.simulation().factory_ground_unit_template(*editable_scene.arena().entities[0].live_entity_index);
    if(!ground||ground->binding!=ArenaGroundUnitBinding::NoTarget||ground->slot[0x7e]!=255)throw std::runtime_error("keyboard ground unit inspection failed");
    // A separate two-player scene exercises the actual M/T input path.
    auto transport_config=construction;
    transport_config.players.push_back({{{3,3},1,false},400,400});
    transport_config.unit_base_available={true,true,false,false};
    ArenaState transport_arena;transport_arena.extent=4;transport_arena.players.resize(2);
    transport_arena.players[0].start_grid=ArenaGridPosition{0,0};
    transport_arena.players[1].start_grid=ArenaGridPosition{3,3};
    NativeConstructionScene transport_scene(transport_arena,transport_config);
    if(transport_scene.enqueue({0,26,2,2})!=ArenaConstructionSubmit::Queued)throw std::runtime_error("transport setup failed");
    transport_scene.step();transport_scene.advance(2);transport_scene.advance(4);
    if(!transport_scene.spawn_reserved_ground_unit(0,40,{1,1}).entity_index)throw std::runtime_error("transport spawn failed");
    const auto before_position=transport_scene.arena().entities[1].position;
    config.construction=&transport_scene;finished.store(false);
    std::thread motion_controls([&]{
        while(!SDL_WasInit(SDL_INIT_VIDEO)){if(finished.load())return;SDL_Delay(1);}
        SDL_Delay(100);SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.sym=SDLK_m;SDL_PushEvent(&key);key.key.keysym.sym=SDLK_n;SDL_PushEvent(&key);key.key.keysym.sym=SDLK_c;SDL_PushEvent(&key);
        for(int i=0;i<2;++i){SDL_Delay(100);key.key.keysym.sym=SDLK_t;SDL_PushEvent(&key);}
        SDL_Delay(100);SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
    });
    const auto motion=run_native_viewer({structure,spawned_model,background},config);
    finished.store(true);motion_controls.join();
    if(!motion)throw std::runtime_error(motion.detail);
    const auto after_position=transport_scene.arena().entities[1].position;
    if(!transport_scene.simulation().ground_transport_enabled() || !transport_scene.simulation().ground_dynamics_enabled() || !transport_scene.simulation().ground_contacts_enabled() ||
       (before_position.x==after_position.x && before_position.z==after_position.z))
        throw std::runtime_error("keyboard transport did not move/project unit");
    // Actual SDL release selection, key-up, owner switch, focus loss and exit.
    ArenaConstructionSimulationConfig input_config;input_config.capacity=8;input_config.seed=4;
    input_config.players={{{{1,1},0,false},100,100},{{{4,1},1,false},100,100}};
    input_config.unit_base_available={true,true,false,false};
    input_config.unit_quotas[0].capacity=3;input_config.unit_quotas[0].pending=3;
    input_config.unit_quotas[0].by_type[0]=2;input_config.unit_quotas[0].by_type[1]=1;
    ArenaState input_arena;input_arena.players.resize(2);
    input_arena.players[0].start_grid=ArenaGridPosition{1,1};input_arena.players[1].start_grid=ArenaGridPosition{4,1};
    NativeConstructionScene input_scene(input_arena,input_config);
    auto second_kind=spawned_model;second_kind.spec.original_slot_index=3;
    config.construction=&input_scene;finished.store(false);
    std::thread release_controls([&]{
        while(!SDL_WasInit(SDL_INIT_VIDEO)){if(finished.load())return;SDL_Delay(1);}
        SDL_Delay(100);
        auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN){SDL_Event e{};e.type=type;e.key.keysym.sym=code;SDL_PushEvent(&e);};
        key(SDLK_e);key(SDLK_d);key(SDLK_t);SDL_Delay(100);
        key(SDLK_d,SDL_KEYUP);key(SDLK_t);SDL_Delay(100);
        key(SDLK_TAB);key(SDLK_d);key(SDLK_TAB);key(SDLK_q);key(SDLK_d);key(SDLK_t);SDL_Delay(100);
        SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&focus);SDL_Delay(100);
        focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;SDL_PushEvent(&focus);key(SDLK_t);SDL_Delay(100);
        key(SDLK_d);key(SDLK_p);key(SDLK_p);SDL_Delay(100);key(SDLK_t);SDL_Delay(100);key(SDLK_d);SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);
    });
    const auto released=run_native_viewer({spawned_model,second_kind,background},config);
    finished.store(true);release_controls.join();
    if(!released)throw std::runtime_error(released.detail);
    if(input_scene.simulation().unit_quotas()[0].active!=2 || input_scene.simulation().unit_quotas()[0].pending!=1 ||
       input_scene.simulation().unit_quotas()[0].by_type[1]!=0)
        throw std::runtime_error("held deployment SDL controls did not consume exactly the selected orders");
    if(input_scene.simulation().ground_deployment()[0]->strength!=0 || input_scene.simulation().ground_deployment()[1]->strength!=0)
        throw std::runtime_error("viewer left deployment held after focus/owner/exit change");
    config.frame_limit=2;config.screenshot=std::filesystem::temp_directory_path()/"comet-render-test.ppm";
    const auto overlay=run_native_viewer({spawned_model,second_kind,background},config);
    if(!overlay)throw std::runtime_error(overlay.detail);
    std::ifstream hud(config.screenshot,std::ios::binary);hud>>magic>>width>>height>>max;hud.get();
    pixels.resize(static_cast<std::size_t>(width)*height*3);hud.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
    int text_pixels=0;for(int y=10;y<17;++y)for(int x=10;x<300;++x){const auto offset=(y*width+x)*3;if(pixels[offset]>150 && pixels[offset+1]>150 && pixels[offset+2]>150)++text_pixels;}
    if(text_pixels<50)throw std::runtime_error("on-screen status text missing");
    config.screenshot=std::filesystem::temp_directory_path()/"comet-render-test.ppm";
    std::filesystem::remove(config.screenshot);std::filesystem::remove(texture);
}
