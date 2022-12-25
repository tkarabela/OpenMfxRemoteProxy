#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"
#include "OpenMfxRemoteProxy/plugin/MfxProxyBroker.h"



OfxExport OfxStatus OfxSetHost(const OfxHost *host) {
    return MfxProxyBroker::instance().OfxSetHost(host);
}

OfxExport int OfxGetNumberOfPlugins() {
    return MfxProxyBroker::instance().OfxGetNumberOfPlugins();
}

OfxExport OfxPlugin *OfxGetPlugin(int nth) {
  static OfxPlugin plugins[] = {
          {
                  /* pluginApi */          kOfxMeshEffectPluginApi,
                  /* apiVersion */         kOfxMeshEffectPluginApiVersion,
                  /* pluginIdentifier */   "MfxSamplePlugin0",
                  /* pluginVersionMajor */ 1,
                  /* pluginVersionMinor */ 0,
                  /* setHost */            plugin0_setHost,
                  /* mainEntry */          plugin0_mainEntry
          },
          {
                  /* pluginApi */          kOfxMeshEffectPluginApi,
                  /* apiVersion */         kOfxMeshEffectPluginApiVersion,
                  /* pluginIdentifier */   "MfxSamplePlugin1",
                  /* pluginVersionMajor */ 1,
                  /* pluginVersionMinor */ 0,
                  /* setHost */            plugin1_setHost,
                  /* mainEntry */          plugin1_mainEntry
          }
  };
  return plugins + nth;
}