cmake_minimum_required(VERSION 3.20)
project(comet_native_recovery LANGUAGES CXX)

# This recovered SPU leaf changes rounding mode for its arithmetic sequence.
if(MSVC)
    set_source_files_properties(src/arena_construction_progress.cpp src/arena_factory_production.cpp src/arena_unit_entity.cpp src/arena_unit_binding.cpp src/arena_player_statistics.cpp src/arena_unit_spawn.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/arena_construction_progress.cpp src/arena_factory_production.cpp src/arena_unit_entity.cpp src/arena_unit_binding.cpp src/arena_player_statistics.cpp src/arena_unit_spawn.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_library(comet_native_recovery STATIC
    src/level_map.cpp
    src/arena_state.cpp
    src/arena_construction.cpp
    src/arena_entity_core.cpp
    src/arena_structure_entity.cpp
    src/arena_entity_storage.cpp
    src/arena_construction_runtime.cpp
    src/arena_factory_production.cpp
    src/arena_unit_entity.cpp
    src/arena_unit_binding.cpp
    src/arena_player_statistics.cpp
    src/arena_unit_spawn.cpp
    src/arena_ground_transport.cpp
    src/arena_ground_steering.cpp
    src/arena_ground_dynamics.cpp
    src/arena_ground_contact.cpp
    src/arena_ground_pair.cpp
    src/arena_ground_terrain.cpp
    src/arena_ground_height.cpp
    src/arena_ground_orientation.cpp
    src/arena_ground_deployment.cpp
    src/arena_ground_lifecycle.cpp
    src/arena_ground_base.cpp
    src/arena_ground_capture.cpp
    src/arena_factory_control.cpp
    src/arena_construction_progress.cpp
    src/arena_construction_simulation.cpp
    src/arena_construction_config.cpp
    src/native_mission_outcome.cpp
    src/native_mission_playback.cpp
    src/arena_health_delta.cpp
    src/arena_converter_activation.cpp
    src/arena_ground_slam.cpp
    src/arena_ground_hop.cpp
    src/arena_area_effect.cpp
    src/arena_ground_slam_effect.cpp
    src/arena_laser_beam.cpp
    src/arena_tesla_runtime.cpp
    src/arena_tesla_source.cpp
    src/arena_cannon_source.cpp
    src/arena_projectile.cpp
    src/arena_impulse.cpp
    src/arena_weapon_aim.cpp
    src/arena_weapon_lead.cpp
    src/arena_weapon_target.cpp
    src/arena_cannon_runtime.cpp

    src/native_deployment_input.cpp
    src/native_construction_scene.cpp
    src/arena_routes.cpp
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

add_executable(comet_arena_ground_transport_tests tests/arena_ground_transport.cpp)
target_link_libraries(comet_arena_ground_transport_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_transport_tests COMMAND comet_arena_ground_transport_tests)
if(MSVC)
    target_compile_options(comet_arena_ground_transport_tests PRIVATE /UNDEBUG)
    set_source_files_properties(src/arena_ground_transport.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    target_compile_options(comet_arena_ground_transport_tests PRIVATE -UNDEBUG)
    set_source_files_properties(src/arena_ground_transport.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

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

add_executable(comet_arena_construction_simulation_tests tests/arena_construction_simulation.cpp)
target_link_libraries(comet_arena_construction_simulation_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_construction_simulation_tests COMMAND comet_arena_construction_simulation_tests)
if(MSVC)
    target_compile_options(comet_arena_construction_simulation_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_construction_simulation_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_unit_binding_tests tests/arena_unit_binding.cpp)
target_link_libraries(comet_arena_unit_binding_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_unit_binding_tests COMMAND comet_arena_unit_binding_tests)
if(MSVC)
    target_compile_options(comet_arena_unit_binding_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_unit_binding_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_unit_entity_tests tests/arena_unit_entity.cpp)
target_link_libraries(comet_arena_unit_entity_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_unit_entity_tests COMMAND comet_arena_unit_entity_tests)
if(MSVC)
    target_compile_options(comet_arena_unit_entity_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_unit_entity_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_factory_production_tests tests/arena_factory_production.cpp)
target_link_libraries(comet_arena_factory_production_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_factory_production_tests COMMAND comet_arena_factory_production_tests)
if(MSVC)
    target_compile_options(comet_arena_factory_production_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_factory_production_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_factory_control_tests tests/arena_factory_control.cpp)
target_link_libraries(comet_arena_factory_control_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_factory_control_tests COMMAND comet_arena_factory_control_tests)
if(MSVC)
    target_compile_options(comet_arena_factory_control_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_factory_control_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_construction_progress_tests tests/arena_construction_progress.cpp)
target_link_libraries(comet_arena_construction_progress_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_construction_progress_tests COMMAND comet_arena_construction_progress_tests)
if(MSVC)
    target_compile_options(comet_arena_construction_progress_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_construction_progress_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_construction_replay src/construction_replay.cpp)
target_link_libraries(comet_construction_replay PRIVATE comet_native_recovery)

add_executable(comet_native src/main.cpp)
target_link_libraries(comet_native PRIVATE comet_native_recovery)
target_compile_features(comet_native PRIVATE cxx_std_20)

add_executable(comet_native_session_tests tests/native_session.cpp)
target_link_libraries(comet_native_session_tests PRIVATE comet_native_recovery)
target_compile_features(comet_native_session_tests PRIVATE cxx_std_20)
add_test(NAME comet_native_session_tests COMMAND comet_native_session_tests)

add_executable(comet_arena_state_tests tests/arena_state.cpp)
target_link_libraries(comet_arena_state_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_state_tests COMMAND comet_arena_state_tests)
add_executable(comet_arena_construction_tests tests/arena_construction.cpp)
target_link_libraries(comet_arena_construction_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_construction_tests COMMAND comet_arena_construction_tests)
add_executable(comet_arena_entity_core_tests tests/arena_entity_core.cpp)
target_link_libraries(comet_arena_entity_core_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_entity_core_tests COMMAND comet_arena_entity_core_tests)
add_executable(comet_arena_structure_entity_tests tests/arena_structure_entity.cpp)
target_link_libraries(comet_arena_structure_entity_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_structure_entity_tests COMMAND comet_arena_structure_entity_tests)
add_executable(comet_arena_entity_storage_tests tests/arena_entity_storage.cpp)
target_link_libraries(comet_arena_entity_storage_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_entity_storage_tests COMMAND comet_arena_entity_storage_tests)
add_executable(comet_arena_construction_runtime_tests tests/arena_construction_runtime.cpp)
target_link_libraries(comet_arena_construction_runtime_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_construction_runtime_tests COMMAND comet_arena_construction_runtime_tests)
add_executable(comet_arena_routes_tests tests/arena_routes.cpp)
target_link_libraries(comet_arena_routes_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_routes_tests COMMAND comet_arena_routes_tests)

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
    add_test(NAME comet_construction_replay_tests COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/construction_replay.py $<TARGET_FILE:comet_construction_replay>)
    add_test(NAME comet_native_cli_tests COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/native_cli.py $<TARGET_FILE:comet_native>)
endif()

add_executable(comet_dds_image_tests tests/dds_image.cpp)
target_link_libraries(comet_dds_image_tests PRIVATE comet_native_recovery)
add_test(NAME comet_dds_image_tests COMMAND comet_dds_image_tests)

# Existing recovery tests use assert; keep checks active in Release CI too.
foreach(test_target comet_native_recovery_smoke comet_obj_face_indices_tests
    comet_obj_polygon_mesh_tests comet_obj_face_line_tests comet_native_session_tests comet_arena_state_tests
    comet_native_model_tests comet_asset_store_tests comet_dds_image_tests comet_arena_construction_tests)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
endforeach()

add_executable(comet_arena_player_statistics_tests tests/arena_player_statistics.cpp)
target_link_libraries(comet_arena_player_statistics_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_player_statistics_tests COMMAND comet_arena_player_statistics_tests)
if(MSVC)
    target_compile_options(comet_arena_player_statistics_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_player_statistics_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_unit_spawn_tests tests/arena_unit_spawn.cpp)
target_link_libraries(comet_arena_unit_spawn_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_unit_spawn_tests COMMAND comet_arena_unit_spawn_tests)
if(MSVC)
    target_compile_options(comet_arena_unit_spawn_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(comet_arena_unit_spawn_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_ground_steering_tests tests/arena_ground_steering.cpp)
target_link_libraries(comet_arena_ground_steering_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_steering_tests COMMAND comet_arena_ground_steering_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_steering_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_steering.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_steering_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_steering.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_dynamics_tests tests/arena_ground_dynamics.cpp)
target_link_libraries(comet_arena_ground_dynamics_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_dynamics_tests COMMAND comet_arena_ground_dynamics_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_dynamics_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_dynamics.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_dynamics_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_dynamics.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_contact_tests tests/arena_ground_contact.cpp)
target_link_libraries(comet_arena_ground_contact_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_contact_tests COMMAND comet_arena_ground_contact_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_contact_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_contact.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_contact_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_contact.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_pair_tests tests/arena_ground_pair.cpp)
target_link_libraries(comet_arena_ground_pair_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_pair_tests COMMAND comet_arena_ground_pair_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_pair_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_pair.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_pair_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_pair.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_terrain_tests tests/arena_ground_terrain.cpp)
target_link_libraries(comet_arena_ground_terrain_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_terrain_tests COMMAND comet_arena_ground_terrain_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_terrain_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_terrain.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_terrain_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_terrain.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_height_tests tests/arena_ground_height.cpp)
target_link_libraries(comet_arena_ground_height_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_height_tests COMMAND comet_arena_ground_height_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_height_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_height.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_height_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_height.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_orientation_tests tests/arena_ground_orientation.cpp)
target_link_libraries(comet_arena_ground_orientation_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_orientation_tests COMMAND comet_arena_ground_orientation_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_orientation_tests PRIVATE /UNDEBUG)
  set_source_files_properties(src/arena_ground_orientation.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  target_compile_options(comet_arena_ground_orientation_tests PRIVATE -UNDEBUG)
  set_source_files_properties(src/arena_ground_orientation.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_capture_tests tests/arena_ground_capture.cpp)
target_link_libraries(comet_arena_ground_capture_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_capture_tests COMMAND comet_arena_ground_capture_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_capture_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_ground_capture_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_arena_ground_base_tests tests/arena_ground_base.cpp)
target_link_libraries(comet_arena_ground_base_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_base_tests COMMAND comet_arena_ground_base_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_base_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_ground_base_tests PRIVATE -UNDEBUG)
endif()

if(MSVC)
  set_source_files_properties(src/arena_ground_base.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  set_source_files_properties(src/arena_ground_base.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_lifecycle_tests tests/arena_ground_lifecycle.cpp)
target_link_libraries(comet_arena_ground_lifecycle_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_lifecycle_tests COMMAND comet_arena_ground_lifecycle_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_lifecycle_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_ground_lifecycle_tests PRIVATE -UNDEBUG)
endif()

if(MSVC)
  set_source_files_properties(src/arena_ground_lifecycle.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  set_source_files_properties(src/arena_ground_lifecycle.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_ground_deployment_tests tests/arena_ground_deployment.cpp)
target_link_libraries(comet_arena_ground_deployment_tests PRIVATE comet_native_recovery)
add_test(NAME comet_arena_ground_deployment_tests COMMAND comet_arena_ground_deployment_tests)
if(MSVC)
  target_compile_options(comet_arena_ground_deployment_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_ground_deployment_tests PRIVATE -UNDEBUG)
endif()

if(MSVC)
  set_source_files_properties(src/arena_ground_deployment.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  set_source_files_properties(src/arena_ground_deployment.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_native_deployment_input_tests tests/native_deployment_input.cpp)
target_link_libraries(comet_native_deployment_input_tests PRIVATE comet_native_recovery)
add_test(NAME comet_native_deployment_input_tests COMMAND comet_native_deployment_input_tests)
if(MSVC)
  target_compile_options(comet_native_deployment_input_tests PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_native_deployment_input_tests PRIVATE -UNDEBUG)
endif()

add_executable(comet_native_mission_outcome_test tests/native_mission_outcome.cpp)
target_link_libraries(comet_native_mission_outcome_test PRIVATE comet_native_recovery)
add_test(NAME comet_native_mission_outcome_test COMMAND comet_native_mission_outcome_test)
if(MSVC)
  target_compile_options(comet_native_mission_outcome_test PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_native_mission_outcome_test PRIVATE -UNDEBUG)
endif()

add_executable(comet_native_mission_playback_test tests/native_mission_playback.cpp)
target_link_libraries(comet_native_mission_playback_test PRIVATE comet_native_recovery)
add_test(NAME comet_native_mission_playback_test COMMAND comet_native_mission_playback_test)
if(MSVC)
  target_compile_options(comet_native_mission_playback_test PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_native_mission_playback_test PRIVATE -UNDEBUG)
endif()

foreach(name arena_health_delta arena_converter_activation arena_ground_health arena_ground_slam arena_ground_hop arena_area_effect arena_ground_slam_effect arena_ground_slam_simulation arena_laser_beam arena_tesla_source)
  add_executable(comet_${name}_test tests/${name}.cpp)
  target_link_libraries(comet_${name}_test PRIVATE comet_native_recovery)
  add_test(NAME comet_${name}_test COMMAND comet_${name}_test)
  if(MSVC)
    target_compile_options(comet_${name}_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(comet_${name}_test PRIVATE -UNDEBUG)
  endif()
endforeach()
if(MSVC)
  set_source_files_properties(src/arena_health_delta.cpp src/arena_converter_activation.cpp src/arena_ground_slam.cpp src/arena_ground_hop.cpp src/arena_area_effect.cpp src/arena_laser_beam.cpp src/arena_tesla_source.cpp src/arena_tesla_runtime.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  set_source_files_properties(src/arena_health_delta.cpp src/arena_converter_activation.cpp src/arena_ground_slam.cpp src/arena_ground_hop.cpp src/arena_area_effect.cpp src/arena_laser_beam.cpp src/arena_tesla_source.cpp src/arena_tesla_runtime.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()

add_executable(comet_arena_tesla_simulation_test tests/arena_tesla_simulation.cpp)
target_link_libraries(comet_arena_tesla_simulation_test PRIVATE comet_native_recovery)
add_test(NAME comet_arena_tesla_simulation_test COMMAND comet_arena_tesla_simulation_test)
if(MSVC)
  target_compile_options(comet_arena_tesla_simulation_test PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_tesla_simulation_test PRIVATE -UNDEBUG)
endif()

foreach(name arena_cannon_simulation arena_cannon_source arena_projectile arena_impulse arena_weapon_aim arena_weapon_lead)
  add_executable(comet_${name}_test tests/${name}.cpp)
  target_link_libraries(comet_${name}_test PRIVATE comet_native_recovery)
  add_test(NAME comet_${name}_test COMMAND comet_${name}_test)
  if(MSVC)
    target_compile_options(comet_${name}_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(comet_${name}_test PRIVATE -UNDEBUG)
  endif()
endforeach()

if(MSVC)
  set_source_files_properties(src/arena_cannon_source.cpp src/arena_projectile.cpp src/arena_impulse.cpp src/arena_weapon_aim.cpp src/arena_weapon_lead.cpp src/arena_weapon_target.cpp src/arena_cannon_runtime.cpp PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
  set_source_files_properties(src/arena_cannon_source.cpp src/arena_projectile.cpp src/arena_impulse.cpp src/arena_weapon_aim.cpp src/arena_weapon_lead.cpp src/arena_weapon_target.cpp src/arena_cannon_runtime.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math;-ffp-contract=off")
endif()
add_executable(comet_arena_cannon_runtime_test tests/arena_cannon_runtime.cpp)
target_link_libraries(comet_arena_cannon_runtime_test PRIVATE comet_native_recovery)
add_test(NAME comet_arena_cannon_runtime_test COMMAND comet_arena_cannon_runtime_test)
if(MSVC)
  target_compile_options(comet_arena_cannon_runtime_test PRIVATE /UNDEBUG)
else()
  target_compile_options(comet_arena_cannon_runtime_test PRIVATE -UNDEBUG)
endif()
