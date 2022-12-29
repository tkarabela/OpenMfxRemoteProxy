#pragma once

#include "OpenMfx/Sdk/Cpp/Host/Host"
#include "OpenMfx/Sdk/Cpp/Host/EffectLibrary"
#include "MfxProxyHostBroker.h"
#include "OpenMfx/Sdk/Cpp/Host/EffectRegistry"


class MfxProxyHost : public OpenMfx::Host {
public:
    MfxProxyHost();
    virtual ~MfxProxyHost();

    void set_remote_plugin_pair_address(const char* pair_address);
    void set_plugin(const char* plugin_path);
    void run_main_loop();

    OpenMfx::EffectRegistry& registry();
    OpenMfx::EffectLibrary& library();

protected:
    MfxProxyHostBroker m_broker;
    OpenMfx::EffectLibrary* m_ofx_library;
};
