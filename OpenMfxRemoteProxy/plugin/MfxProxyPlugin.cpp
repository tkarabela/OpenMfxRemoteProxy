#include "MfxProxyPlugin.h"
#include <cassert>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>


MfxProxyPlugin::MfxProxyPlugin(std::array<OfxPlugin, MFX_PROXY_MAX_NUMBER_OF_PLUGINS> *ofx_plugins)
: m_broker(),
  m_ofx_host(nullptr),
  m_ofx_plugins(ofx_plugins) {
}

void MfxProxyPlugin::OfxPluginSetHost(int effect_id, OfxHost *host) {
    assert(effect_id >= 0 && effect_id < m_effects.size());
    m_effects[effect_id].SetHost(host);
}

OfxStatus
MfxProxyPlugin::OfxPluginMainEntry(int effect_id, const char *action, const void *handle, OfxPropertySetHandle inArgs,
                                   OfxPropertySetHandle outArgs) {
    assert(effect_id >= 0 && effect_id < m_effects.size());
    return m_effects[effect_id].MainEntry(action, handle, inArgs, outArgs);
}

OfxPlugin* MfxProxyPlugin::OfxGetPlugin(int effect_id) {
    assert(effect_id >= 0 && effect_id < MFX_PROXY_MAX_NUMBER_OF_PLUGINS);
    return &(*m_ofx_plugins)[effect_id];
}

int MfxProxyPlugin::OfxGetNumberOfPlugins() {
    return static_cast<int>(m_effects.size());
}

OfxStatus MfxProxyPlugin::OfxSetHost(const OfxHost *host, const char *bundle_dir) {
    m_ofx_host = host;

    // get bundle directory
    if (!bundle_dir) {
        // TODO get bundle dir from host and remove the bundle_dir argument
        ERR_LOG << "[MfxProxyPlugin] missing bundle_dir in OfxSetHost";
        return kOfxStatFailed;
    }

    m_bundle_directory = std::string(bundle_dir);

    // setup broker
    try {
        m_broker.setup_sockets();
    } catch (zmq::error_t &err) {
        ERR_LOG << "[MfxProxyEffect] ZMQ error when setting up broker sockets: " << err.what();
        return kOfxStatFailed;
    }

    // start broker thread
    {
        DEBUG_LOG << "[MfxProxyPlugin] Starting broker thread";
    }
    m_broker.start_thread();

    // read configuration file
    auto bundle_dir_path = boost::filesystem::path(bundle_dir);
    auto config_path = bundle_dir_path / "OpenMfxRemoteProxy.ini";
    if (!boost::filesystem::exists(config_path)) {
        ERR_LOG << "[MfxProxyEffect] Missing config file: " << config_path;
        return kOfxStatFailed;
    }


    boost::property_tree::ptree config_tree;
    std::string plugin_path;
    try {
        boost::property_tree::read_ini(config_path.string(), config_tree);
        plugin_path = config_tree.get<std::string>("plugin_path");
    } catch (std::runtime_error &err) {
        ERR_LOG << "[MfxProxyEffect] Failed when reading config file: " << config_path << " - " << err.what();
        return kOfxStatFailed;
    }

    // find proxy host binary
    #if defined(WIN32) || defined(WIN64)
    boost::filesystem::path proxy_host_path = bundle_dir_path / "OpenMfxRemoteProxy_Host.exe";
    #else
    boost::filesystem::path proxy_host_path = bundle_dir_path / "OpenMfxRemoteProxy_Host";
    #endif

    if (!boost::filesystem::exists(proxy_host_path)) {
        ERR_LOG << "[MfxProxyEffect] Missing proxy host runner binary: " << proxy_host_path;
        return kOfxStatFailed;
    }

    // launch subprocess
    {
        LOG << "[MfxProxyEffect] Starting proxy host process: "
            << proxy_host_path.c_str() << " "
            << plugin_path << " "
            << m_broker.pair_address();
    }
    boost::process::child proxy_host_process(proxy_host_path.c_str(), plugin_path, m_broker.pair_address());
    proxy_host_process.detach();
    // TODO the broker could periodically check up if the process is still running and poison the plugin if not

    // setup effects
    auto plugin_definition = m_broker.get_plugin_definition();
    if (!plugin_definition) {
        ERR_LOG << "[MfxProxyEffect] Failed to get plugin definition from broker!";
        return kOfxStatFailed;
    }

    assert(plugin_definition->number_of_plugins == plugin_definition->plugins.size());
    for (int i = 0; i < plugin_definition->number_of_plugins; i++) {
        if (i > MFX_PROXY_MAX_NUMBER_OF_PLUGINS) {
            WARN_LOG << "[MfxProxyEffect] Exceeded MFX_PROXY_MAX_NUMBER_OF_PLUGINS, ignoring effect #" << i;
            continue;
        }

        const auto& plugin_def = plugin_definition->plugins[i];
        {
            LOG << "[MfxProxyEffect] Defining effect #" << i << " (" << plugin_def.identifier << ")";
        }
        assert(plugin_def.effect_id == i);

        // fill in OfxPlugin struct
        (*m_ofx_plugins)[i].pluginVersionMajor = plugin_def.version_major;
        (*m_ofx_plugins)[i].pluginVersionMinor = plugin_def.version_minor;
        (*m_ofx_plugins)[i].pluginIdentifier = plugin_def.identifier.c_str();

        // create MfxProxyEffect instance
        m_effects.emplace_back(i, &m_broker);
        m_effects[i].SetName(plugin_def.identifier.c_str());
    }

    return kOfxStatOK;
}
