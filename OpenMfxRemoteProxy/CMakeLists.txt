cmake_minimum_required(VERSION 3.24)

project(OpenMfxRemoteProxy)
set(CMAKE_CXX_STANDARD 17)

find_package(cppzmq CONFIG REQUIRED)
add_definitions(-DZMQ_BUILD_DRAFT_API)  # https://github.com/zeromq/cppzmq/issues/175#issuecomment-903966668

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.74 REQUIRED)


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
        common/*.h
        plugin/*.cpp
        plugin/*.h
)
file(GLOB MFX_REMOTE_PROXY_PLUGIN_INC
        common
        plugin
        ${MFX_REMOTE_PROXY_FLATBUFFERS_INC}
)
set(MFX_REMOTE_PROXY_PLUGIN_LIB
        OpenMfxRemoteProxy_Flatbuffers
        cppzmq-static
        Boost::boost
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        OpenMfx::Sdk::Cpp::Plugin
)

add_library(OpenMfxRemoteProxy_Plugin SHARED ${MFX_REMOTE_PROXY_PLUGIN_SRC})
target_include_directories(OpenMfxRemoteProxy_Plugin PRIVATE ${MFX_REMOTE_PROXY_PLUGIN_INC})
target_link_libraries(OpenMfxRemoteProxy_Plugin PRIVATE ${MFX_REMOTE_PROXY_PLUGIN_LIB})
#target_link_libraries(OpenMfxRemoteProxy_Plugin PUBLIC  Boost::filesystem)  # XXX
set_target_properties(OpenMfxRemoteProxy_Plugin PROPERTIES SUFFIX ".ofx")
set_property(TARGET OpenMfxRemoteProxy_Plugin PROPERTY POSITION_INDEPENDENT_CODE ON)


# -----------------------------------------------------------------------------
# OpenMfxRemoteProxy - proxy host executable
# -----------------------------------------------------------------------------

file(GLOB MFX_REMOTE_PROXY_HOST_SRC
        common/*.cpp
        common/*.h
        host/*.cpp
        host/*.h
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
add_dependencies(OpenMfxRemoteProxy_Plugin OpenMfxRemoteProxy_Host)

# -----------------------------------------------------------------------------
# OpenMfxRemoteProxy - dummy host for testing
# -----------------------------------------------------------------------------

set(MFX_REMOTE_PROXY_DUMMY_HOST_LIB
        OpenMfx::Core
        OpenMfx::Sdk::Cpp::Common
        OpenMfx::Sdk::Cpp::Host
        OpenMfx::Sdk::Cpp::Plugin
        )

add_executable(OpenMfxRemoteProxy_DummyHost ./test/dummy_host.cpp)
target_link_libraries(OpenMfxRemoteProxy_DummyHost PRIVATE ${MFX_REMOTE_PROXY_DUMMY_HOST_LIB})
add_dependencies(OpenMfxRemoteProxy_DummyHost OpenMfxRemoteProxy_Plugin OpenMfxRemoteProxy_Host)
