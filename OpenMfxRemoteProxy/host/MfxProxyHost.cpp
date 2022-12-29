#include "MfxProxyHost.h"

MfxProxyHost::MfxProxyHost()
: m_ofx_library(nullptr) {
    m_broker.set_host(this);
}

MfxProxyHost::~MfxProxyHost() {
    if (m_ofx_library) {
        registry().releaseLibrary(m_ofx_library);
    }
}

void MfxProxyHost::set_plugin(const char *plugin_path) {
    registry().setHost(this);
    m_ofx_library = registry().getLibrary(plugin_path);
}

OpenMfx::EffectRegistry &MfxProxyHost::registry() {
    return OpenMfx::EffectRegistry::GetInstance();
}

void MfxProxyHost::set_remote_plugin_pair_address(const char *pair_address) {
    m_broker.setup_sockets(pair_address);
}

void MfxProxyHost::run_main_loop() {
    m_broker.start_thread();
    m_broker.join_thread();
}

OpenMfx::EffectLibrary &MfxProxyHost::library() {
    assert(m_ofx_library);
    return *m_ofx_library;
}
