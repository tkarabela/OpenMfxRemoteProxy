#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"
#include "MfxProxyPlugin.h"

// adapted from MfxRegister() macro
static MfxProxyPlugin* g_MfxProxyPlugin = nullptr;

template <size_t N>
static void StaticOfxPluginSetHost(OfxHost *host) {
    return g_MfxProxyPlugin->OfxPluginSetHost(N, host);
}

template <size_t N>
static OfxStatus StaticOfxPluginMainEntry(const char *action, const void *handle, OfxPropertySetHandle inArgs,
                                          OfxPropertySetHandle outArgs) {
    return g_MfxProxyPlugin->OfxPluginMainEntry(N, action, handle, inArgs, outArgs);
}


template <size_t X>
static constexpr OfxPlugin make_plugin() {
    return {
            /* pluginApi */          kOfxMeshEffectPluginApi,
            /* apiVersion */         kOfxMeshEffectPluginApiVersion,
            /* pluginIdentifier */   "MfxProxyPluginNameUnfilled",
            /* pluginVersionMajor */ 1,
            /* pluginVersionMinor */ 0,
            /* setHost */            StaticOfxPluginSetHost<X>,
            /* mainEntry */          StaticOfxPluginMainEntry<X>
    };
}

template <size_t ... Is>
static constexpr auto make_plugins_array(std::index_sequence<Is...>) {
    return std::array<OfxPlugin, MFX_PROXY_MAX_NUMBER_OF_PLUGINS> { make_plugin<Is>()... };
}

std::array<OfxPlugin, MFX_PROXY_MAX_NUMBER_OF_PLUGINS> gOfxPlugins(
        make_plugins_array(std::make_index_sequence<MFX_PROXY_MAX_NUMBER_OF_PLUGINS>()));

extern "C" {

OfxExport OfxStatus OfxSetHost(const OfxHost *host) {
    if (!g_MfxProxyPlugin) {
        g_MfxProxyPlugin = new MfxProxyPlugin(&gOfxPlugins);
    }
    return g_MfxProxyPlugin->OfxSetHost(host);
}

OfxExport int OfxGetNumberOfPlugins() {
    return g_MfxProxyPlugin->OfxGetNumberOfPlugins();
}

OfxExport OfxPlugin *OfxGetPlugin(int nth) {
    if (nth < MFX_PROXY_MAX_NUMBER_OF_PLUGINS) {
        return &gOfxPlugins[nth];
    } else {
        assert(0);
        return nullptr;
    }
}

// TODO remove this when https://github.com/eliemichel/OpenMfx/issues/4 is fixed
OfxExport void OfxSetBundleDirectory(const char *path) {
    if (!g_MfxProxyPlugin) {
        g_MfxProxyPlugin = new MfxProxyPlugin(&gOfxPlugins);
    }
    auto status = g_MfxProxyPlugin->OfxSetHost(nullptr, path);
    assert(status == kOfxStatOK);
}

}
