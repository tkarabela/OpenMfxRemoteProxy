cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(OpenMfxRemoteProxy
        HOMEPAGE_URL "https://github.com/tkarabela/OpenMfxRemoteProxy"
        LANGUAGES C CXX
)

add_subdirectory(OpenMfx)
add_subdirectory(flatbuffers ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build EXCLUDE_FROM_ALL)  # TODO when vcpkg flatbuffers supports CMake `flatbuffers_generate_headers`, use it instead
add_subdirectory(OpenMfxRemoteProxy)
