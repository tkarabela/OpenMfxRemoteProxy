cmake_minimum_required(VERSION 3.24)

project(OpenMfxRemoteProxy)
set(CMAKE_CXX_STANDARD 17)

find_package(cppzmq CONFIG REQUIRED)


# -----------------------------------------------------------------------------
# OpenMfxRemoteProxy - flatbuffers
# -----------------------------------------------------------------------------

get_target_property(MFX_REMOTE_PROXY_FLATBUFFERS_INC
        FlatBuffers INCLUDE_DIRECTORIES
)

set(MFX_REMOTE_PROXY_FLATBUFFERS_SRC
        common/serialization/MfxFlatbuffersBasicTypes.fbs
        common/serialization/MfxFlatbuffersMessages.fbs
)

flatbuffers_generate_headers(
        TARGET OpenMfxRemoteProxy_Flatbuffers
        SCHEMAS ${MFX_REMOTE_PROXY_FLATBUFFERS_SRC}
)

# -----------------------------------------------------------------------------
# OpenMfxRemoteProxy - .ofx plugin
# -----------------------------------------------------------------------------

file(GLOB MFX_REMOTE_PROXY_PLUGIN_SRC
        common/*.cpp
        plugin/*.cpp
)
file(GLOB MFX_REMOTE_PROXY_PLUGIN_INC
        common
        plugin
        ${MFX_REMOTE_PROXY_FLATBUFFERS_INC}
)
set(MFX_REMOTE_PROXY_PLUGIN_LIB
        OpenMfxRemoteProxy_Flatbuffers
        cppzmq-static
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        OpenMfx::Sdk::Cpp::Plugin
)

add_library(OpenMfxRemoteProxy_Plugin SHARED ${MFX_REMOTE_PROXY_PLUGIN_SRC})
target_include_directories(OpenMfxRemoteProxy_Plugin PRIVATE ${MFX_REMOTE_PROXY_PLUGIN_INC})
target_link_libraries(OpenMfxRemoteProxy_Plugin PRIVATE ${MFX_REMOTE_PROXY_PLUGIN_LIB})
set_target_properties(OpenMfxRemoteProxy_Plugin PROPERTIES SUFFIX ".ofx")
set_property(TARGET OpenMfxRemoteProxy_Plugin PROPERTY POSITION_INDEPENDENT_CODE ON)


# -----------------------------------------------------------------------------
# OpenMfxRemoteProxy - proxy host executable
# -----------------------------------------------------------------------------

file(GLOB MFX_REMOTE_PROXY_HOST_SRC
        common/*.cpp
        host/*.cpp
)
file(GLOB MFX_REMOTE_PROXY_HOST_INC
        common
        host
        ${MFX_REMOTE_PROXY_FLATBUFFERS_INC}
)
set(MFX_REMOTE_PROXY_HOST_LIB
        OpenMfxRemoteProxy_Flatbuffers
        cppzmq-static
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        OpenMfx::Sdk::Cpp::Plugin
)

add_executable(OpenMfxRemoteProxy_Host ${MFX_REMOTE_PROXY_HOST_SRC})
target_include_directories(OpenMfxRemoteProxy_Host PRIVATE ${MFX_REMOTE_PROXY_HOST_INC})
target_link_libraries(OpenMfxRemoteProxy_Host PRIVATE ${MFX_REMOTE_PROXY_HOST_LIB})
