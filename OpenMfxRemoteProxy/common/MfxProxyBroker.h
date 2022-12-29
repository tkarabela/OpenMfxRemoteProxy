#pragma once

#include <zmq.hpp>
#include <thread>

class MfxProxyBroker {
public:
    MfxProxyBroker();
    MfxProxyBroker(const MfxProxyBroker&) = delete;
    MfxProxyBroker& operator=(const MfxProxyBroker&) = delete;

    zmq::context_t& ctx() {
        return m_ctx;
    }

    virtual const char * pub_address() const = 0;
    virtual const char * sub_address() const = 0;

    virtual void broker_thread_main() = 0;
    void start_thread();

protected:
    zmq::context_t m_ctx;
    zmq::socket_t m_pub_socket;
    zmq::socket_t m_sub_socket;
    zmq::socket_t m_pair_socket;
    std::thread m_thread;
};
