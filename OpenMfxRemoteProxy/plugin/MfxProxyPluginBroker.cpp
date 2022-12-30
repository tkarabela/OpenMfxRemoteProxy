#include "MfxProxyPluginBroker.h"
#include "OpenMfx/Sdk/Cpp/Common"
#include "MfxProxyEffect.h"
#include <chrono>

using namespace std::chrono_literals;

void MfxProxyPluginBroker::setup_sockets() {
    m_pub_socket.bind(pub_address());
    LOG << "[" << broker_name() << "] PUB socket bind to " << m_pub_socket.get(zmq::sockopt::last_endpoint);

    m_sub_socket.bind(sub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, "");
    LOG << "[" << broker_name() << "] SUB socket bind to " << m_sub_socket.get(zmq::sockopt::last_endpoint);

    m_pair_socket.bind("tcp://127.0.0.1:*");
    m_pair_address = m_pair_socket.get(zmq::sockopt::last_endpoint);
    LOG << "[" << broker_name() << "] PAIR socket bind to " << m_pair_address;
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
        zmq::message_t message;
        auto res = m_pair_socket.recv(message);
        MfxProxyMessage message_(std::move(message));
        auto data = message_.message_as<OpenMfxRemoteProxy::RemoteHostStarted>();

        {
            DEBUG_LOG << "[" << broker_name() << "] Received RemoteHostStarted message";
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
            zmq::message_t message;
            auto res = events[i].socket.recv(message, zmq::recv_flags::none);
            MfxProxyMessage message_(std::move(message));

            DEBUG_LOG << "[" << broker_name() << "]"
                      << " Received message " << OpenMfxRemoteProxy::EnumNameMessage(message_.message_type())
                      << " from " << broker_socket_to_string(socket_enum) << ":"
                      << " message_id=" << message_.message_id()
                      << " message_thread_id=" << message_.message_thread_id()
                      << " effect_id=" << message_.effect_id();

            switch (socket_enum) {
                case BrokerSocket::pair:
                {
                    send_message_pub(std::move(message_));
                    break;
                }
                case BrokerSocket::sub:
                {
                    send_message_pair(std::move(message_));
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

uint32_t MfxProxyPluginBroker::generate_message_thread_id() {
    return m_rng();
}

constexpr const char *MfxProxyPluginBroker::broker_name() const {
    return "MfxProxyPluginBroker";
}
