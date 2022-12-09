cmake_minimum_required(VERSION 3.24)

add_subdirectory(OpenMfx)

project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(INC
        .
        ./OpenMfx/include
        ./OpenMfx/sdk/cpp/common/include
        ./OpenMfx/sdk/cpp/host/include)

# OpenMfx dependency
set(LIB
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        )

add_executable(untitled main.cpp)
target_include_directories(untitled PRIVATE ${INC})
target_link_libraries(untitled PRIVATE ${LIB})
