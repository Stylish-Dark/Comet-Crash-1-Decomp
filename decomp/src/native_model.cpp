#include "comet/native_model.hpp"
#include "comet/obj_face_line.hpp"
#include "comet/obj_polygon_mesh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <utility>
namespace comet::decomp {
namespace {
AssetLoadResult fail(std::size_t line, std::string detail) { return {false, line, std::move(detail)}; }
ModelPosition cross(ModelPosition a, ModelPosition b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
ModelPosition subtract(ModelPosition a, ModelPosition b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
ModelPosition normalized(ModelPosition p) {
    const auto length = std::hypot(p.x,p.y,p.z);
    return length > 0 && std::isfinite(length) ? ModelPosition{p.x/length,p.y/length,p.z/length} : ModelPosition{0,1,0};
}
bool number(std::istream& in, float& value) { return bool(in >> value) && std::isfinite(value); }
bool position(std::istream& in, ModelPosition& p) { return number(in,p.x) && number(in,p.y) && number(in,p.z); }
std::string remaining(std::istream& in) {
    std::string value; std::getline(in >> std::ws,value);
    const auto last=value.find_last_not_of(" \t\r");
    if (last==std::string::npos) return {};
    value.resize(last+1); return value;
}
}
AssetLoadResult parse_obj_model(std::istream& input, ModelLoadParameters parameters,
    std::uint32_t options, float vertex_y_offset, NativeModel& output) {
    if (!std::isfinite(parameters.geometry_scale) || !std::isfinite(parameters.signed_radius_scale) ||
        !std::isfinite(vertex_y_offset)) return fail(0,"nonfinite model load parameter");
    NativeModel next;
    std::vector<ModelPosition> positions, normals;
    std::vector<std::array<float,2>> texcoords;
    ObjTriangleMeshBuilder builder;
    std::vector<std::vector<ObjFaceVertex>> polygons;
    std::string material;
    auto flush = [&]() -> bool {
        if (polygons.empty()) return true;
        if (!builder.append_submesh(polygons)) return false;
        next.submeshes.push_back({builder.ranges().back(),material,{}});
        polygons.clear(); return true;
    };
    std::string line; std::size_t line_number=0;
    while (std::getline(input,line)) {
        ++line_number;
        line.resize(line.find('#')==std::string::npos ? line.size() : line.find('#'));
        std::istringstream words(line); words.imbue(std::locale::classic());
        std::string directive; if (!(words >> directive)) continue;
        if (directive == "v") {
            ModelPosition p;
            if (!position(words,p)) return fail(line_number,"invalid vertex position");
            p=apply_optional_vertex_y_offset(apply_geometry_scale(p,parameters.geometry_scale),options,vertex_y_offset);
            if (!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)) return fail(line_number,"vertex transform overflow");
            positions.push_back(p);
        } else if (directive == "vn") {
            ModelPosition p; if (!position(words,p)) return fail(line_number,"invalid normal");
            normals.push_back(normalized(p));
        } else if (directive == "vt") {
            float u=0,v=0; if (!number(words,u)) return fail(line_number,"invalid texture coordinate");
            if (words >> std::ws && words.peek()!=std::char_traits<char>::eof() && !number(words,v))
                return fail(line_number,"invalid texture coordinate");
            texcoords.push_back({u,v});
        } else if (directive == "f") {
            auto face=parse_obj_face_line(line,{positions.size(),texcoords.size(),normals.size()});
            if (!face) return fail(line_number,"invalid face reference or polygon");
            polygons.push_back(std::move(face.vertices));
        } else if (directive == "usemtl" || directive == "g" || directive == "o") {
            if (!flush()) return fail(line_number,"mesh exceeds index limits");
            if (directive == "usemtl") { material=remaining(words); if (material.empty()) return fail(line_number,"empty material name"); }
        } else if (directive == "mtllib") {
            std::string path; while (words >> path) next.material_libraries.emplace_back(path);
        }
    }
    if (input.bad()) return fail(line_number,"OBJ stream read failure");
    if (!flush()) return fail(line_number,"mesh exceeds index limits");
    if (builder.indices().empty()) return fail(line_number,"model contains no triangles");
    next.indices=builder.indices();
    next.vertices.reserve(builder.vertices().size());
    std::vector<bool> missing_normals;
    for (const auto& ref : builder.vertices()) {
        NativeVertex vertex{}; vertex.position=positions[ref.position];
        if (ref.normal) vertex.normal=normals[*ref.normal];
        if (ref.texcoord) { vertex.u=texcoords[*ref.texcoord][0]; vertex.v=texcoords[*ref.texcoord][1]; }
        next.vertices.push_back(vertex); missing_normals.push_back(!ref.normal);
    }
    // Port policy: generate area-weighted normals only for corners without vn.
    for (std::size_t i=0;i<next.indices.size();i+=3) {
        auto a=next.indices[i],b=next.indices[i+1],c=next.indices[i+2];
        const auto n=cross(subtract(next.vertices[b].position,next.vertices[a].position),
                           subtract(next.vertices[c].position,next.vertices[a].position));
        for (auto index : {a,b,c}) if (missing_normals[index]) {
            auto& dst=next.vertices[index].normal; dst.x+=n.x;dst.y+=n.y;dst.z+=n.z;
        }
    }
    next.bounds_min=next.bounds_max=next.vertices.front().position;
    for (std::size_t i=0;i<next.vertices.size();++i) {
        auto& v=next.vertices[i]; if (missing_normals[i]) v.normal=normalized(v.normal);
        next.bounds_min.x=std::min(next.bounds_min.x,v.position.x);next.bounds_min.y=std::min(next.bounds_min.y,v.position.y);next.bounds_min.z=std::min(next.bounds_min.z,v.position.z);
        next.bounds_max.x=std::max(next.bounds_max.x,v.position.x);next.bounds_max.y=std::max(next.bounds_max.y,v.position.y);next.bounds_max.z=std::max(next.bounds_max.z,v.position.z);
    }
    next.signed_bounding_radius=compute_signed_bounding_radius(positions,parameters.signed_radius_scale);
    if (!std::isfinite(next.signed_bounding_radius)) return fail(line_number,"bounding radius overflow");
    output=std::move(next);return {};
}
AssetLoadResult parse_mtl_materials(std::istream& input, std::vector<NativeMaterial>& output) {
    std::vector<NativeMaterial> next; std::string line; std::size_t line_number=0;
    while (std::getline(input,line)) {
        ++line_number; if (auto comment=line.find('#');comment!=std::string::npos) line.resize(comment);
        std::istringstream words(line);words.imbue(std::locale::classic());std::string directive;
        if (!(words>>directive)) continue;
        if (directive=="newmtl") {
            auto name=remaining(words); if (name.empty()) return fail(line_number,"empty material name");
            NativeMaterial m{};m.name=name;m.properties.use_team_color=material_name_uses_team_color(name);
            next.push_back(std::move(m));continue;
        }
        if (next.empty()) continue;
        auto& m=next.back();
        if (directive=="Ka"||directive=="Kd"||directive=="Ks") {
            auto& c=directive=="Ka" ? m.properties.ambient : directive=="Kd" ? m.properties.diffuse : m.properties.specular;
            if (!number(words,c.r)||!number(words,c.g)||!number(words,c.b)) return fail(line_number,"invalid material color");
        } else if (directive=="Ns") {
            float value; if (!number(words,value)) return fail(line_number,"invalid specular exponent");
            m.properties.scaled_specular_exponent=scale_mtl_specular_exponent(value);
            if (!std::isfinite(m.properties.scaled_specular_exponent)) return fail(line_number,"specular exponent overflow");
        } else if (directive=="map_Kd"||directive=="map_Ks"||directive=="bump"||directive=="map_bump"||directive=="cube") {
            std::string path; words >> path; if (path.empty()) return fail(line_number,"empty texture path");
            auto& destination=directive=="map_Kd" ? m.diffuse_texture : directive=="map_Ks" ? m.specular_texture : directive=="cube" ? m.cube_texture : m.bump_texture;
            destination=path;
        }
    }
    if (input.bad()) return fail(line_number,"MTL stream read failure");
    output=std::move(next);return {};
}
} // namespace comet::decomp
