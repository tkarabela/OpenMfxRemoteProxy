#pragma once

#include <zmq.hpp>
#include <random>
#include <condition_variable>
#include <mutex>
#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"
#include "MfxProxyBroker.h"
#include "flatbuffers/flatbuffer_builder.h"


class MfxProxyHost;

class MfxProxyHostBroker : public MfxProxyBroker {
public:
    ~MfxProxyHostBroker() override = default;

    const char * pub_address() const override;
    const char * sub_address() const override;
    constexpr const char * broker_name() const override;
    void broker_thread_main() override;

    void setup_sockets(const char *pair_address);
    void set_host(MfxProxyHost* host);
    MfxProxyHost& host();

protected:
    std::string m_pair_address;
    MfxProxyHost *m_host;
};
