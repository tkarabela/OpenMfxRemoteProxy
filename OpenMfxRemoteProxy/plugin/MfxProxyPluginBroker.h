#pragma once

#include <zmq.hpp>
#include <random>
#include <condition_variable>
#include <mutex>
#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"
#include "MfxProxyBroker.h"
//#include "OpenMfxRemoteProxy/common/MfxProxyBroker.h"

class MfxProxyPluginBroker : public MfxProxyBroker {
public:

    // this mirrors the `RemoteHostStarted` message in flatbuffers
    struct BrokerPluginBundleDefinition {
        struct BrokerPluginDefinition {
            int effect_id;
            std::string identifier;
            int version_major;
            int version_minor;
        };

        int number_of_plugins;
        std::vector<BrokerPluginDefinition> plugins;
    };

    OfxStatus OfxSetHost(const OfxHost* host);
    int OfxGetNumberOfPlugins();
    OfxPlugin* OfxGetPlugin(int nth);

    virtual ~MfxProxyPluginBroker() = default;

    const char * pub_address() const override {
        return "inproc://plugin_broker_pub";
    }

    const char * sub_address() const override {
        return "inproc://plugin_broker_sub";
    }

    const char * pair_address() const;

    void setup_sockets();

    void broker_thread_main() override;

    const BrokerPluginBundleDefinition* get_plugin_definition();

    uint64_t generate_message_thread_id() {
        return m_rng();
    }

protected:
    std::string m_pair_address;
    std::mt19937_64 m_rng;

    std::condition_variable m_plugin_definition_cv;
    std::mutex m_plugin_definition_mutex;
    std::unique_ptr<BrokerPluginBundleDefinition> m_plugin_definition;
};
