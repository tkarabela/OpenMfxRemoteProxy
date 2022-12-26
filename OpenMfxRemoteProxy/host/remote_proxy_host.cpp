#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "OpenMfx/Sdk/Cpp/Host/MeshEffect"
#include "OpenMfx/Sdk/Cpp/Host/MeshProps"
#include "OpenMfx/Sdk/Cpp/Host/Mesh"
#include <zmq.hpp>
#include <iostream>
#include "openmfx_remote_proxy_messages_generated.h"

int main(int argc, char *argv[]) {
    std::cout << "Hello from remote_proxy_host\n";

    flatbuffers::FlatBufferBuilder builder(1024);
    auto plugin_name = builder.CreateString("foo");
    std::vector<flatbuffers::Offset<OpenMfxRemoteProxy::OfxMesh>> inputs;

    OpenMfx::Host host;
    OpenMfx::EffectRegistry& registry = OpenMfx::EffectRegistry::GetInstance();
    registry.setHost(&host);
    auto library = registry.getLibrary("./cmake-build-debug/OpenMfx/sdk/examples/c/plugins/OpenMfx_Example_C_Plugin_color_to_uv.ofx");

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::push);
    sock.bind("inproc://test");
    sock.send(zmq::str_buffer("Hello, world"), zmq::send_flags::dontwait);

    return 0;
}
