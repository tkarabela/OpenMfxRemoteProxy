#pragma once

#include <zmq.hpp>
#include <thread>
#include "MfxProxyMessage.h"

class MfxProxyBroker {
public:
    enum class BrokerSocket { pub, sub, pair };

    MfxProxyBroker();
    MfxProxyBroker(const MfxProxyBroker&) = delete;
    MfxProxyBroker& operator=(const MfxProxyBroker&) = delete;
    virtual ~MfxProxyBroker() = default;

    zmq::context_t& ctx() {
        return m_ctx;
    }

    virtual const char * pub_address() const = 0;
    virtual const char * sub_address() const = 0;
    virtual const char * broker_name() const = 0;

    virtual void broker_thread_main() = 0;
    void start_thread();
    void join_thread();

    void send_message_pub(MfxProxyMessage&& message);
    void send_message_pair(MfxProxyMessage&& message);

protected:
    zmq::context_t m_ctx;
    zmq::socket_t m_pub_socket;
    zmq::socket_t m_sub_socket;
    zmq::socket_t m_pair_socket;
    std::thread m_thread;
};

constexpr const char* broker_socket_to_string(MfxProxyBroker::BrokerSocket e) noexcept {
    switch (e) {
        case MfxProxyBroker::BrokerSocket::pub: return "PUB";
        case MfxProxyBroker::BrokerSocket::sub: return "SUB";
        case MfxProxyBroker::BrokerSocket::pair: return "PAIR";
    }
    return "";
}
