cmake_minimum_required(VERSION 3.24)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

# XXX why is this needed?
set(cppzmq_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/share/cppzmq")
set(ZeroMQ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-linux/share/zeromq")

add_subdirectory(OpenMfx)
add_subdirectory(OpenMfxRemoteProxy)

project(untitled)

find_package(cppzmq CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

set(INC
        .
        ./include
        ./OpenMfx/include
        ./OpenMfx/sdk/cpp/common/include
        ./OpenMfx/sdk/cpp/host/include
        ./OpenMfx/sdk/cpp/plugin/include)

# OpenMfx dependency
set(LIB
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        OpenMfx::Sdk::Cpp::Plugin
        )

add_executable(untitled main.cpp OpenMfxRemoteProxy/plugin/MfxProxyBroker.cpp OpenMfxRemoteProxy/plugin/MfxProxyBroker.h OpenMfxRemoteProxy/plugin/remote_proxy_plugin.cpp OpenMfxRemoteProxy/plugin/MfxProxyEffect.h OpenMfxRemoteProxy/plugin/MfxProxyEffect.cpp)
target_include_directories(untitled PRIVATE ${INC})
target_link_libraries(untitled PRIVATE ${LIB})
target_link_libraries(untitled PRIVATE cppzmq-static)
