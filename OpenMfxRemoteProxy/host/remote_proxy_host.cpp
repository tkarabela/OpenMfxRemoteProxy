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
#include "MfxProxyHost.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: remote_proxy_host plugin.ofx tcp://127.0.0.1:12345\n";
        return 1;
    }

    auto plugin_path = argv[1];
    auto pair_address = argv[2];

    {
        LOG << "[MfxProxyHost] Hello from remote_proxy_host";
    }

    try {
        MfxProxyHost host;
        host.set_remote_plugin_pair_address(pair_address);
        host.set_plugin(plugin_path);
        host.run_main_loop();
    } catch (std::exception &err) {
        ERR_LOG << "[MfxProxyHost] Terminating due to unexpected error: " << err.what();
        return 1;
    }

    {
        LOG << "[MfxProxyHost] Finished, exiting";
    }

    return 0;
}
