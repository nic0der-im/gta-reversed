cmake_minimum_required(VERSION 3.20)
project(librw_sa_spike C CXX)

# Standalone spike: prove that librw (GL3) runs on macOS/arm64 and can load and
# render GTA San Andreas DFF/TXD assets. Not part of the game build.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# librw is pinned to a known-good commit (see FINDINGS.md).
set(LIBRW_COMMIT "18532c2e13efbc1aa43f0b00b4459831f9414de0" CACHE STRING "librw git commit")
set(LIBRW_PLATFORM "GL3" CACHE STRING "" FORCE)
set(LIBRW_GL3_GFXLIB "SDL3" CACHE STRING "" FORCE)
set(LIBRW_TOOLS OFF CACHE BOOL "" FORCE)
set(LIBRW_INSTALL OFF CACHE BOOL "" FORCE)
set(LIBRW_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(librw
    GIT_REPOSITORY https://github.com/aap/librw.git
    GIT_TAG        ${LIBRW_COMMIT}
)
FetchContent_MakeAvailable(librw)

add_executable(librw_spike
    src/main.cpp
    src/img.cpp
)

target_include_directories(librw_spike PRIVATE src)
target_link_libraries(librw_spike PRIVATE librw::librw)
target_compile_definitions(librw_spike PRIVATE LIBRW_COMMIT="${LIBRW_COMMIT}")
