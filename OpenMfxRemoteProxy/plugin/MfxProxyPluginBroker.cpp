#include "MfxProxyPluginBroker.h"
#include "OpenMfx/Sdk/Cpp/Common"
#include "MfxProxyEffect.h"
#include <chrono>

using namespace std::chrono_literals;

void MfxProxyPluginBroker::setup_sockets() {
    m_pub_socket.bind(pub_address());
    LOG << "[MfxProxyPlugin] Broker PUB socket bind to " << m_pub_socket.get(zmq::sockopt::last_endpoint);

    m_sub_socket.bind(sub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, "");
    LOG << "[MfxProxyPlugin] Broker SUB socket bind to " << m_sub_socket.get(zmq::sockopt::last_endpoint);

    m_pair_socket.bind("tcp://127.0.0.1:*");
    m_pair_address = m_pair_socket.get(zmq::sockopt::last_endpoint);
    LOG << "[MfxProxyPlugin] Broker PAIR socket bind to " << m_pair_address;
}

const char *MfxProxyPluginBroker::pair_address() const {
    return m_pair_address.c_str();
}

void MfxProxyPluginBroker::broker_thread_main() {
    {
        LOG << "[MfxProxyPlugin] Hello from broker thread";
    }

    // receive RemoteHostStarted
    {
        zmq::message_t msg;
        auto res = m_pair_socket.recv(msg);
        FlatbufferRecvMessage msg_(std::move(msg));
        auto data = msg_.message_envelope()->message_as_RemoteHostStarted();
        assert(data);

        {
            DEBUG_LOG << "[MfxProxyPlugin] Broker received RemoteHostStarted message";
        }

        auto plugin_definition = std::make_unique<BrokerPluginBundleDefinition>();
        plugin_definition->number_of_plugins = data->number_of_plugins();
        for (auto p : *data->plugins()) {
            BrokerPluginBundleDefinition::BrokerPluginDefinition plugin;
            plugin.identifier = p->identifier()->str();
            plugin.version_major = p->version_major();
            plugin.version_minor = p->version_minor();
            plugin.effect_id = p->effect_id();

            plugin_definition->plugins.emplace_back(plugin);
        }

        {
            std::lock_guard<std::mutex> lk(m_plugin_definition_mutex);
            m_plugin_definition.swap(plugin_definition);
            m_plugin_definition_cv.notify_all();
        }
    }

    // main loop
    zmq::poller_t<BrokerSocket> poller;
    auto sub_socket_enum = BrokerSocket::sub;
    auto pair_socket_enum = BrokerSocket::pair;
    poller.add(m_pair_socket, zmq::event_flags::pollin, &pair_socket_enum);
    poller.add(m_sub_socket, zmq::event_flags::pollin, &sub_socket_enum);

    std::vector<zmq::poller_event<BrokerSocket>> events(2);

    while (true) {
        auto n = poller.wait_all(events, 60000ms);
        if (!n) continue;

        for (int i = 0; i < n; i++) {
            auto socket_enum = *events[i].user_data;
            DEBUG_LOG << "[MfxProxyPlugin] Broker got message from socket " << broker_socket_to_string(socket_enum);
            zmq::message_t msg;
            events[i].socket.recv(msg, zmq::recv_flags::none);

            switch (socket_enum) {
                case BrokerSocket::pair:
                {
                    DEBUG_LOG << "[MfxProxyPlugin] Forwarding message to socket SUB";
                    m_sub_socket.send(std::move(msg), zmq::send_flags::none);
                    break;
                }
                case BrokerSocket::sub:
                {
                    DEBUG_LOG << "[MfxProxyPlugin] Forwarding message to socket PAIR";
                    m_pair_socket.send(std::move(msg), zmq::send_flags::none);
                    break;
                }
                default:
                {
                    ERR_LOG << "[MfxProxyPlugin] unexpected broker socket!";
                    assert(0);
                }
            }
        }
    }
}

const MfxProxyPluginBroker::BrokerPluginBundleDefinition *MfxProxyPluginBroker::get_plugin_definition() {
    // m_plugin_definition is filled at start of broker_thread_main based on received message from proxy host

    std::unique_lock<std::mutex> lk(m_plugin_definition_mutex);
    auto status = m_plugin_definition_cv.wait_for(lk, 5000ms);
    if (status == std::cv_status::timeout) {
        ERR_LOG << "[MfxProxyPlugin] Timeout when getting plugin definition from proxy host!";
    }

    return m_plugin_definition.get();
}
