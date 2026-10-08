#pragma once

#include "comet/obj_face_indices.hpp"

#include <string_view>
#include <vector>

namespace comet::decomp {

// Parse one complete OBJ face line ("f ..."), including whitespace and an
// optional trailing comment. Counts must be captured at the face's position
// in the source file; negative indices are relative to those counts.
enum class ObjFaceLineError {
    None,
    NotFaceDirective,
    TooFewVertices,
    InvalidReference,
};

struct ObjFaceLine {
    std::vector<ObjFaceVertex> vertices;
    ObjFaceLineError error = ObjFaceLineError::None;
    ObjReferenceError reference_error = ObjReferenceError::None;
    explicit operator bool() const { return error == ObjFaceLineError::None; }
};

ObjFaceLine parse_obj_face_line(std::string_view line, ObjSourceCounts counts);

} // namespace comet::decomp
