#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "OpenMfx/Sdk/Cpp/Host/MeshEffect"
#include "OpenMfx/Sdk/Cpp/Host/MeshProps"
#include "OpenMfx/Sdk/Cpp/Host/Mesh"
#include <zmq.hpp>
#include <iostream>
#include "MfxFlatbuffersMessages_generated.h"
#include "MfxFlatbuffersBasicTypes_generated.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: remote_proxy_host plugin.ofx tcp://127.0.0.1:12345\n";
        return 1;
    }

    auto plugin_path = argv[1];
    auto pair_address = argv[2];

    std::cout << "Hello from remote_proxy_host\n";

    OpenMfx::Host host;
    OpenMfx::EffectRegistry& registry = OpenMfx::EffectRegistry::GetInstance();
    registry.setHost(&host);
    auto library = registry.getLibrary(plugin_path);

    return 0;
}
