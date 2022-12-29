#pragma once

#include "OpenMfx/Sdk/Cpp/Common"
#include "MfxProxyPluginBroker.h"
#include "MfxProxyEffect.h"

// TODO increase max number of plugins
#define MFX_PROXY_MAX_NUMBER_OF_PLUGINS 4

class MfxProxyPlugin {
public:
    MfxProxyPlugin(const MfxProxyPlugin&) = delete;
    MfxProxyPlugin& operator=(const MfxProxyPlugin&) = delete;

    OfxStatus OfxSetHost(const OfxHost *host, const char *bundle_dir = nullptr);
    int OfxGetNumberOfPlugins();
    OfxPlugin* OfxGetPlugin(int effect_id);

    void OfxPluginSetHost(int effect_id, OfxHost *host);

    OfxStatus OfxPluginMainEntry(int effect_id, const char *action, const void *handle, OfxPropertySetHandle inArgs,
                                 OfxPropertySetHandle outArgs);

    explicit MfxProxyPlugin(std::array<OfxPlugin, MFX_PROXY_MAX_NUMBER_OF_PLUGINS> *ofx_plugins);

protected:
    MfxProxyPluginBroker m_broker;
    const OfxHost* m_ofx_host;
    std::string m_bundle_directory;
    std::array<OfxPlugin, MFX_PROXY_MAX_NUMBER_OF_PLUGINS> *m_ofx_plugins;
    std::vector<MfxProxyEffect> m_effects;
};
