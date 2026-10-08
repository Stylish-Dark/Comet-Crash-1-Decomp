#include "comet/native_session.hpp"
#include "comet/asset_store.hpp"
#ifdef COMET_NATIVE_VIEWER
#include "comet/native_viewer.hpp"
#endif
#include <charconv>
#include <iostream>
#include <string_view>
namespace {
bool integer(std::string_view text,std::uint32_t& value){auto p=std::from_chars(text.data(),text.data()+text.size(),value);return p.ec==std::errc{}&&p.ptr==text.data()+text.size();}
int usage(){std::cerr<<"Usage: comet_native [map-directory [level-id]] [--assets game-data-root]\n"
    <<"                    [--validate-assets | --viewer] [--model index] [--frames count]\n"
    <<"                    [--screenshot output.ppm] [--object relative-model.obj] [--hidden]\n";return 2;}
}
int main(int argc,char**argv){
    using namespace comet::decomp;
    std::filesystem::path maps,assets,screenshot,object;
    std::uint32_t level=0,model=0,frames=0;bool viewer=false,validate=false,hidden=false,level_set=false;
    for(int i=1;i<argc;++i){
        const std::string_view arg=argv[i];
        if(arg=="--viewer")viewer=true;
        else if(arg=="--validate-assets")validate=true;
        else if(arg=="--hidden")hidden=true;
        else if(arg=="--assets"||arg=="--screenshot"||arg=="--model"||arg=="--frames"||arg=="--object"){
            if(++i>=argc)return usage();
            if(arg=="--assets")assets=argv[i];
            else if(arg=="--object")object=argv[i];
            else if(arg=="--screenshot")screenshot=argv[i];
            else if(!integer(argv[i],arg=="--model"?model:frames))return usage();
        }else if(arg.starts_with("--"))return usage();
        else if(maps.empty())maps=argv[i];
        else if(!level_set){if(!integer(arg,level))return usage();level_set=true;}
        else return usage();
    }
    if((maps.empty()&&assets.empty())||((viewer||validate||!object.empty())&&assets.empty())||
        (!viewer&&(frames||model||hidden||!screenshot.empty())))return usage();
    try{
        if(!maps.empty()){
            NativeSessionConfig config;config.data_root=maps;NativeSession session;
            const auto result=initialize_native_session(config,level,session);
            if(!result){std::cerr<<result.detail<<'\n';return 1;}
            std::cout<<"Comet Crash native arena bootstrap\nLevel: "<<session.current_level_id<<"\nExtent: "<<session.level.arena_extent
                <<"\nPrimary records: "<<session.level.primary_records.size()<<"\nSecondary records: "<<session.level.secondary_records.size()
                <<"\nModel asset entries: "<<session.model_assets.size()<<"\nRender targets: "<<session.targets.textures.size()<<'\n';
        }
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
            if(!result){std::cerr<<result.detail<<" (line "<<result.line<<")\n";return 1;}
            std::size_t triangles=0,vertices=0;
            for(const auto&m:models){triangles+=m.model.indices.size()/3;vertices+=m.model.vertices.size();}
            std::cout<<"Loaded "<<models.size()<<" arena model entries, "<<vertices<<" vertices, "<<triangles<<" triangles\n";
            if(viewer){
#ifdef COMET_NATIVE_VIEWER
                NativeViewerConfig config;config.initial_model=model;config.frame_limit=frames;config.hidden=hidden;config.screenshot=screenshot;
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
