#pragma once

#include <zmq.hpp>
#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"

class MfxProxyBroker {
public:
    MfxProxyBroker(const MfxProxyBroker&) = delete;
    MfxProxyBroker& operator=(const MfxProxyBroker&) = delete;

    static MfxProxyBroker& instance() {
        if (!MfxProxyBroker::INSTANCE) {
            MfxProxyBroker::INSTANCE = new MfxProxyBroker {};
        }
        return *MfxProxyBroker::INSTANCE;
    }

    OfxStatus OfxSetHost(const OfxHost* host);
    int OfxGetNumberOfPlugins();
    OfxPlugin* OfxGetPlugin(int nth);

    zmq::context_t& ctx() {
        return m_ctx;
    }

    const char * pub_address() const {
        return "inproc://plugin_broker_pub";
    }

    const char * sub_address() const {
        return "inproc://plugin_broker_sub";
    }


private:
    MfxProxyBroker() = default;


    zmq::context_t m_ctx;
    zmq::socket_t m_pub_socket;
    zmq::socket_t m_sub_socket;
    zmq::socket_t m_pair_socket;

    static MfxProxyBroker* INSTANCE;
};
