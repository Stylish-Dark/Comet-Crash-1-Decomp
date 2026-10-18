#include "comet/native_session.hpp"
#include "comet/asset_store.hpp"
#ifdef COMET_NATIVE_VIEWER
#include "comet/native_viewer.hpp"
#endif
#include "comet/arena_construction_config.hpp"
#include "comet/native_construction_scene.hpp"
#include <fstream>
#include <memory>
#include <charconv>
#include <iostream>
#include <string_view>
namespace {
bool integer(std::string_view text,std::uint32_t& value){auto p=std::from_chars(text.data(),text.data()+text.size(),value);return p.ec==std::errc{}&&p.ptr==text.data()+text.size();}
int usage(){std::cerr<<"Usage: comet_native [map-directory [level-id]] [--assets game-data-root]\n"
    <<"                    [--validate-assets | --viewer | --arena] [--construction-config file] [--model index] [--frames count]\n"
    <<"                    [--skirmish-config file] [--screenshot output.ppm] [--object relative-model.obj] [--hidden]\n";return 2;}
}
int main(int argc,char**argv){
    using namespace comet::decomp;
    std::filesystem::path maps,assets,screenshot,object,construction_config,skirmish_config;
    std::uint32_t level=0,model=0,frames=0;bool viewer=false,arena=false,validate=false,hidden=false,level_set=false;
    NativeSession session;
    for(int i=1;i<argc;++i){
        const std::string_view arg=argv[i];
        if(arg=="--viewer")viewer=true;
        else if(arg=="--arena"){arena=true;viewer=true;}
        else if(arg=="--validate-assets")validate=true;
        else if(arg=="--hidden")hidden=true;
        else if(arg=="--assets"||arg=="--screenshot"||arg=="--model"||arg=="--frames"||arg=="--object"||arg=="--construction-config"||arg=="--skirmish-config"){
            if(++i>=argc)return usage();
            if(arg=="--skirmish-config")skirmish_config=argv[i];
            else if(arg=="--construction-config")construction_config=argv[i];
            else if(arg=="--assets")assets=argv[i];
            else if(arg=="--object")object=argv[i];
            else if(arg=="--screenshot")screenshot=argv[i];
            else if(!integer(argv[i],arg=="--model"?model:frames))return usage();
        }else if(arg.starts_with("--"))return usage();
        else if(maps.empty())maps=argv[i];
        else if(!level_set){if(!integer(arg,level))return usage();level_set=true;}
        else return usage();
    }
    if(!skirmish_config.empty())arena=true;
    if((!skirmish_config.empty()&&(!maps.empty()||!construction_config.empty()||!object.empty())) ||
        (arena&&((maps.empty()&&skirmish_config.empty())||!object.empty()))||
        (maps.empty()&&assets.empty()&&skirmish_config.empty())||((viewer||validate||!object.empty())&&assets.empty())||
        (!viewer&&(frames||model||hidden||!screenshot.empty())))return usage();
    try{
        std::optional<ArenaConstructionSimulationConfig> construction;
        std::unique_ptr<NativeConstructionScene> scene;
        if(!construction_config.empty()||!skirmish_config.empty()){
            if(skirmish_config.empty()&&!arena)throw std::invalid_argument("construction config requires --arena");
            std::ifstream input(skirmish_config.empty()?construction_config:skirmish_config);if(!input)throw std::invalid_argument("cannot open construction config");std::size_t line=0;
            construction=read_arena_construction_config(input,line);
            for(std::string text;std::getline(input,text);){text=text.substr(0,text.find('#'));if(text.find_first_not_of(" \t\r")!=std::string::npos)throw std::invalid_argument("viewer config cannot contain scenario commands");}
        }
        if(!skirmish_config.empty()) {
            const auto result=initialize_native_skirmish({},*construction,session);
            if(!result){std::cerr<<result.detail<<'\n';return 1;}
            std::cout<<"Native skirmish bootstrap\nExtent: "<<session.arena.extent
                <<"\nArena entities: "<<session.arena.entities.size()<<'\n';
        }
        if(!maps.empty()){
            NativeSessionConfig config;config.data_root=maps;if(construction){config.arena.player_count=construction->players.size();config.arena.secondary_player=construction->players.size()>1?1:-1;}
            const auto result=initialize_native_session(config,level,session);
            if(!result){std::cerr<<result.detail<<'\n';return 1;}
            std::cout<<"Comet Crash native arena bootstrap\nLevel: "<<session.current_level_id<<"\nExtent: "<<session.level.arena_extent
                <<"\nPrimary records: "<<session.level.primary_records.size()<<"\nSecondary records: "<<session.level.secondary_records.size()
                <<"\nArena entities: "<<session.arena.entities.size()<<"\nUnsupported primary records: "<<session.arena.unsupported_records.size()
                <<"\nModel asset entries: "<<session.model_assets.size()<<"\nRender targets: "<<session.targets.textures.size()<<'\n';
        }
        if(construction){scene=std::make_unique<NativeConstructionScene>(session.arena,*construction);std::cout<<"Live map structures: "<<scene->imports().live<<"; display-only: "<<scene->imports().display_only<<'\n';}
        if(!assets.empty()){
            AssetStore store(assets);std::vector<ArenaNativeModel> models;
            const std::string object_name=object.generic_string();
            AssetLoadResult result;
            if(object.empty()) result=load_arena_models(store,models);
            else {
                ArenaNativeModel selected{ {object_name,0,0,0,1,0}, {} };
                result=store.load_model(object,{},0,0,selected.model);
                if(result) models.push_back(std::move(selected));
            }
            if(result&&arena){std::vector<ArenaNativeModel> environment;result=load_arena_environment_models(store,session.arena.theme,environment);if(result)for(auto& m:environment)models.push_back(std::move(m));}
            if(!result){std::cerr<<result.detail<<" (line "<<result.line<<")\n";return 1;}
            std::size_t triangles=0,vertices=0;
            for(const auto&m:models){triangles+=m.model.indices.size()/3;vertices+=m.model.vertices.size();}
            std::cout<<"Loaded "<<models.size()<<" arena model entries, "<<vertices<<" vertices, "<<triangles<<" triangles\n";
            if(viewer){
#ifdef COMET_NATIVE_VIEWER
                NativeViewerConfig config;config.initial_model=model;config.frame_limit=frames;config.hidden=hidden;config.screenshot=screenshot;config.arena=arena?&session.arena:nullptr;config.construction=scene.get();
                const auto rendered=run_native_viewer(models,config);
                if(!rendered){std::cerr<<rendered.detail<<'\n';return 1;}
#else
                std::cerr<<"Graphics viewer disabled; rebuild with -DCOMET_NATIVE_VIEWER=ON\n";return 1;
#endif
            }
        }
    }catch(const std::exception&ex){std::cerr<<ex.what()<<'\n';return 1;}
    return 0;
}
