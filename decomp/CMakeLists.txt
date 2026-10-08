cmake_minimum_required(VERSION 3.20)
project(comet_native_recovery LANGUAGES CXX)

add_library(comet_native_recovery STATIC
    src/level_map.cpp
    src/native_session.cpp
    src/arena_render_targets.cpp
    src/arena_assets.cpp
    src/material_shader_policy.cpp
    src/model_geometry.cpp
    src/obj_face_indices.cpp
    src/obj_polygon_mesh.cpp
    src/obj_face_line.cpp
    src/material_textures.cpp
    src/material_properties.cpp
    src/model_load_parameters.cpp
)

target_include_directories(comet_native_recovery PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(comet_native_recovery PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(comet_native_recovery PRIVATE /W4 /permissive-)
else()
    target_compile_options(comet_native_recovery PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(comet_native_recovery_smoke
    tests/smoke.cpp
)
target_link_libraries(comet_native_recovery_smoke PRIVATE comet_native_recovery)
target_compile_features(comet_native_recovery_smoke PRIVATE cxx_std_20)

add_test(NAME comet_native_recovery_smoke COMMAND comet_native_recovery_smoke)

add_executable(comet_obj_face_indices_tests tests/obj_face_indices.cpp)
target_link_libraries(comet_obj_face_indices_tests PRIVATE comet_native_recovery)
target_compile_features(comet_obj_face_indices_tests PRIVATE cxx_std_20)
add_test(NAME comet_obj_face_indices_tests COMMAND comet_obj_face_indices_tests)

add_executable(comet_obj_polygon_mesh_tests tests/obj_polygon_mesh.cpp)
target_link_libraries(comet_obj_polygon_mesh_tests PRIVATE comet_native_recovery)
target_compile_features(comet_obj_polygon_mesh_tests PRIVATE cxx_std_20)
add_test(NAME comet_obj_polygon_mesh_tests COMMAND comet_obj_polygon_mesh_tests)

add_executable(comet_obj_face_line_tests tests/obj_face_line.cpp)
target_link_libraries(comet_obj_face_line_tests PRIVATE comet_native_recovery)
target_compile_features(comet_obj_face_line_tests PRIVATE cxx_std_20)
add_test(NAME comet_obj_face_line_tests COMMAND comet_obj_face_line_tests)

add_executable(comet_native src/main.cpp)
target_link_libraries(comet_native PRIVATE comet_native_recovery)
target_compile_features(comet_native PRIVATE cxx_std_20)

add_executable(comet_native_session_tests tests/native_session.cpp)
target_link_libraries(comet_native_session_tests PRIVATE comet_native_recovery)
target_compile_features(comet_native_session_tests PRIVATE cxx_std_20)
add_test(NAME comet_native_session_tests COMMAND comet_native_session_tests)
