cmake_minimum_required(VERSION 3.20)
project(comet_native_recovery LANGUAGES CXX)

add_library(comet_native_recovery STATIC
    src/level_map.cpp
    src/native_session.cpp
    src/native_model.cpp
    src/dds_image.cpp
    src/asset_store.cpp
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

add_executable(comet_native_model_tests tests/native_model.cpp)
target_link_libraries(comet_native_model_tests PRIVATE comet_native_recovery)
add_test(NAME comet_native_model_tests COMMAND comet_native_model_tests)

add_executable(comet_asset_store_tests tests/asset_store.cpp)
target_link_libraries(comet_asset_store_tests PRIVATE comet_native_recovery)
add_test(NAME comet_asset_store_tests COMMAND comet_asset_store_tests)

option(COMET_NATIVE_VIEWER "Build the SDL2/OpenGL native asset viewer" OFF)
if(COMET_NATIVE_VIEWER)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    add_library(comet_native_viewer STATIC src/native_viewer.cpp)
    target_link_libraries(comet_native_viewer PUBLIC comet_native_recovery SDL2::SDL2 OpenGL::GL)
    target_compile_definitions(comet_native PRIVATE COMET_NATIVE_VIEWER=1)
    target_link_libraries(comet_native PRIVATE comet_native_viewer)
    add_executable(comet_native_viewer_tests tests/native_viewer.cpp)
    target_link_libraries(comet_native_viewer_tests PRIVATE comet_native_viewer)
    add_test(NAME comet_native_viewer_tests COMMAND comet_native_viewer_tests)
    set_tests_properties(comet_native_viewer_tests PROPERTIES LABELS "graphics" ENVIRONMENT "SDL_VIDEODRIVER=offscreen")
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_test(NAME comet_native_cli_tests COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/native_cli.py $<TARGET_FILE:comet_native>)
endif()

add_executable(comet_dds_image_tests tests/dds_image.cpp)
target_link_libraries(comet_dds_image_tests PRIVATE comet_native_recovery)
add_test(NAME comet_dds_image_tests COMMAND comet_dds_image_tests)

# Existing recovery tests use assert; keep checks active in Release CI too.
foreach(test_target comet_native_recovery_smoke comet_obj_face_indices_tests
    comet_obj_polygon_mesh_tests comet_obj_face_line_tests comet_native_session_tests
    comet_native_model_tests comet_asset_store_tests comet_dds_image_tests)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
endforeach()
