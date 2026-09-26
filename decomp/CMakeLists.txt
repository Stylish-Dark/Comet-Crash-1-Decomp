cmake_minimum_required(VERSION 3.20)
project(comet_native_recovery LANGUAGES CXX)

add_library(comet_native_recovery STATIC
    src/level_map.cpp
    src/arena_render_targets.cpp
    src/arena_assets.cpp
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
