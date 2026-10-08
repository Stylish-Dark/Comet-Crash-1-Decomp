#include "comet/asset_store.hpp"
#include <fstream>
#include <unordered_map>
#include <utility>
namespace comet::decomp {
AssetStore::AssetStore(std::filesystem::path root) : root_(std::filesystem::weakly_canonical(root)) {}
std::optional<std::filesystem::path> AssetStore::resolve(const std::filesystem::path& relative) const {
    if (relative.empty() || relative.has_root_directory() || relative.has_root_name()) return {};
    std::error_code error;
    const auto path=std::filesystem::weakly_canonical(root_/relative,error);
    if (error) return {};
    auto root_it=root_.begin(),path_it=path.begin();
    for (;root_it!=root_.end();++root_it,++path_it)
        if (path_it==path.end() || *path_it!=*root_it) return {};
    return path;
}
AssetLoadResult AssetStore::load_model(const std::filesystem::path& relative,
    ModelLoadParameters parameters,std::uint32_t options,float vertex_y_offset,NativeModel& output) const {
    const auto path=resolve(relative);
    if (!path) return {false,0,"model path leaves asset root: "+relative.string()};
    std::ifstream input(*path);
    if (!input) return {false,0,"cannot open model: "+path->string()};
    NativeModel next;
    auto result=parse_obj_model(input,parameters,options,vertex_y_offset,next);
    if (!result) { result.detail=path->string()+": "+result.detail;return result; }
    std::unordered_map<std::string,NativeMaterial> materials;
    for (const auto& library : next.material_libraries) {
        const auto mtl_path=resolve(relative.parent_path()/library);
        if (!mtl_path) return {false,0,"material library leaves asset root"};
        std::ifstream mtl(*mtl_path);
        if (!mtl) return {false,0,"cannot open material library: "+mtl_path->string()};
        std::vector<NativeMaterial> loaded;
        result=parse_mtl_materials(mtl,loaded);
        if (!result) { result.detail=mtl_path->string()+": "+result.detail;return result; }
        for (auto& material : loaded) {
            for (auto* texture : {&material.diffuse_texture,&material.specular_texture,&material.bump_texture,&material.cube_texture}) {
                if (texture->empty()) continue;
                // Shipped exporters retain .tif names and one absolute author path.
                // The recovered parser overwrites the final three letters with dds.
                // Absolute author paths are treated as basenames, never host paths.
                auto texture_name=*texture;
                if (texture_name.has_root_directory() || texture_name.has_root_name()) texture_name=texture_name.filename();
                texture_name.replace_extension(".dds");
                const auto resolved=resolve((relative.parent_path()/library).parent_path()/texture_name);
                if (!resolved) return {false,0,"texture path leaves asset root: "+texture->string()+" in "+mtl_path->string()};
                *texture=*resolved;
            }
            const auto name=material.name;materials.insert_or_assign(name,std::move(material));
        }
    }
    for (auto& mesh : next.submeshes) {
        if (mesh.material_name.empty()) { mesh.material.properties.diffuse={0.65f,0.65f,0.65f,0}; continue; }
        const auto material=materials.find(mesh.material_name);
        if (material==materials.end()) return {false,0,"undefined material: "+mesh.material_name};
        mesh.material=material->second;
    }
    output=std::move(next); return {};
}
AssetLoadResult load_arena_models(const AssetStore& assets,std::vector<ArenaNativeModel>& output) {
    std::vector<ArenaNativeModel> next;
    for (const auto& spec : arena_model_asset_manifest()) {
        ArenaNativeModel model{spec,{}};
        const auto result=assets.load_model(spec.path,{spec.geometry_scale,spec.signed_radius_scale},spec.original_loader_options,0,model.model);
        if (!result) return result;
        next.push_back(std::move(model));
    }
    output=std::move(next);return {};
}
} // namespace comet::decomp
