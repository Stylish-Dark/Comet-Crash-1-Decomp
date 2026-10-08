#include "comet/obj_face_line.hpp"

namespace comet::decomp {
namespace {
bool space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\f' || c == '\v';
}
} // namespace

ObjFaceLine parse_obj_face_line(std::string_view line, ObjSourceCounts counts) {
    ObjFaceLine result;
    auto pos = line.find_first_not_of(" \t\r\n\f\v");
    if (pos == std::string_view::npos || line[pos] != 'f' ||
        (pos + 1 < line.size() && !space(line[pos + 1]) &&
         line[pos + 1] != '#')) {
        result.error = ObjFaceLineError::NotFaceDirective;
        return result;
    }
    ++pos;
    while (pos < line.size()) {
        while (pos < line.size() && space(line[pos])) ++pos;
        if (pos == line.size() || line[pos] == '#') break;
        const auto start = pos;
        while (pos < line.size() && !space(line[pos]) && line[pos] != '#') ++pos;
        const auto reference = parse_obj_face_vertex(
            line.substr(start, pos - start), counts);
        if (!reference) {
            result.vertices.clear();
            result.error = ObjFaceLineError::InvalidReference;
            result.reference_error = reference.error;
            return result;
        }
        result.vertices.push_back(reference.vertex);
        if (pos < line.size() && line[pos] == '#') break;
    }
    if (result.vertices.size() < 3) {
        result.vertices.clear();
        result.error = ObjFaceLineError::TooFewVertices;
    }
    return result;
}
} // namespace comet::decomp
