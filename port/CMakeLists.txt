cmake_minimum_required(VERSION 3.20)
project(CometCrashPC VERSION 0.1.0 LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "CometCrashPC currently targets Windows only")
endif()
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED PS3RECOMP_DIR)
    message(FATAL_ERROR "Set -DPS3RECOMP_DIR to a ps3recomp checkout")
endif()
if(NOT DEFINED RECOMP_DIR)
    set(RECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../generated/recompiled")
endif()
if(NOT DEFINED SPU_LIFTED_DIR)
    set(SPU_LIFTED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../generated/spu")
endif()
if(NOT DEFINED SPU_REGISTRY)
    set(SPU_REGISTRY "${CMAKE_CURRENT_SOURCE_DIR}/../generated/spu_workloads.c")
endif()
get_filename_component(PS3RECOMP_DIR "${PS3RECOMP_DIR}" ABSOLUTE)
get_filename_component(RECOMP_DIR "${RECOMP_DIR}" ABSOLUTE)
get_filename_component(SPU_LIFTED_DIR "${SPU_LIFTED_DIR}" ABSOLUTE)
get_filename_component(SPU_REGISTRY "${SPU_REGISTRY}" ABSOLUTE)

if(NOT EXISTS "${RECOMP_DIR}/ppu_recomp.h")
    message(FATAL_ERROR "No ppu_recomp.h in ${RECOMP_DIR}; run tools/comet_port.py lift first")
endif()
file(GLOB RECOMP_SOURCES CONFIGURE_DEPENDS "${RECOMP_DIR}/*.c" "${RECOMP_DIR}/*.cpp")
if(NOT RECOMP_SOURCES)
    message(FATAL_ERROR "No lifted PPU sources in ${RECOMP_DIR}")
endif()

file(GLOB SPU_LIFTED CONFIGURE_DEPENDS "${SPU_LIFTED_DIR}/*/spu_recomp.c")
if(NOT EXISTS "${SPU_REGISTRY}")
    message(FATAL_ERROR "No SPU registry at ${SPU_REGISTRY}; Comet Crash has active embedded SPU workloads")
endif()
if(NOT SPU_LIFTED)
    message(FATAL_ERROR "No lifted SPU sources in ${SPU_LIFTED_DIR}")
endif()
list(APPEND SPU_SOURCES "${SPU_REGISTRY}" ${SPU_LIFTED})

add_subdirectory("${PS3RECOMP_DIR}" "${CMAKE_BINARY_DIR}/ps3recomp_sdk" EXCLUDE_FROM_ALL)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(HLE_NID_TABLE "${CMAKE_CURRENT_BINARY_DIR}/ppu_hle_nids.cpp")
add_custom_command(
    OUTPUT "${HLE_NID_TABLE}"
    COMMAND "${Python3_EXECUTABLE}" "${PS3RECOMP_DIR}/tools/gen_hle_nids.py" --out "${HLE_NID_TABLE}" --all
    DEPENDS "${PS3RECOMP_DIR}/tools/gen_hle_nids.py" "${PS3RECOMP_DIR}/tools/nid_database.py"
    COMMENT "Generating ps3recomp HLE NID table")

set(PS3RECOMP_SCAFFOLD
    "${PS3RECOMP_DIR}/runtime/ppu/ppu_loader.cpp"
    "${PS3RECOMP_DIR}/runtime/ppu/ppu_hle.cpp"
    "${PS3RECOMP_DIR}/runtime/ppu/ppu_fs.cpp"
    "${PS3RECOMP_DIR}/runtime/ppu/ppu_sysprx.cpp")

add_executable(CometCrashPC
    main.cpp stubs.cpp comet_host.cpp comet_settings.cpp comet_compat.cpp
    ${PS3RECOMP_SCAFFOLD} ${HLE_NID_TABLE} ${RECOMP_SOURCES} ${SPU_SOURCES})
target_include_directories(CometCrashPC PRIVATE
    "${RECOMP_DIR}" "${PS3RECOMP_DIR}/include" "${PS3RECOMP_DIR}/runtime/ppu"
    "${PS3RECOMP_DIR}/runtime/spu" "${PS3RECOMP_DIR}/runtime/memory"
    "${PS3RECOMP_DIR}/runtime/syscalls" "${PS3RECOMP_DIR}/runtime/platform"
    "${PS3RECOMP_DIR}/libs/video" "${PS3RECOMP_DIR}/libs/input"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(CometCrashPC PRIVATE
    ps3recomp_runtime
    d3d12 dxgi d3dcompiler uuid dxguid
    ws2_32 xinput ole32 bcrypt
    dbghelp user32 gdi32 winmm)
set_target_properties(CometCrashPC PROPERTIES LINKER_LANGUAGE CXX)
if(MSVC)
    target_compile_options(CometCrashPC PRIVATE
        /W3 /wd4100 /wd4244 /wd4267 /Zc:__cplusplus
        /clang:-fno-strict-aliasing
        /clang:-ffp-contract=off)
    set(RECOMP_COMPILE_OPTIONS "/w;/bigobj")
else()
    target_compile_options(CometCrashPC PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        -fno-strict-aliasing
        -ffp-contract=off)
    set(RECOMP_COMPILE_OPTIONS "-w;-Wa,-mbig-obj")
endif()
target_compile_definitions(CometCrashPC PRIVATE _CRT_SECURE_NO_WARNINGS)
set_source_files_properties(${RECOMP_SOURCES} PROPERTIES COMPILE_OPTIONS "${RECOMP_COMPILE_OPTIONS}")
set_source_files_properties(${SPU_SOURCES} PROPERTIES
    INCLUDE_DIRECTORIES "${PS3RECOMP_DIR}/runtime/spu"
    COMPILE_DEFINITIONS SPU_USE_DISPATCH_MT
    COMPILE_OPTIONS "-O2")
install(TARGETS CometCrashPC RUNTIME DESTINATION bin)
