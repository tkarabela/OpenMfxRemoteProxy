#include "MfxProxyHostBroker.h"
#include "MfxProxyHost.h"
#include "OpenMfx/Sdk/Cpp/Common"
#include "flatbuffers/flatbuffer_builder.h"
#include "MfxFlatbuffersMessages_generated.h"
#include "MfxFlatbuffersBasicTypes_generated.h"
#include "zmq.hpp"
#include "MfxProxyMessage.h"
#include <chrono>
using namespace std::chrono_literals;

const char *MfxProxyHostBroker::pub_address() const {
    return "inproc://plugin_host_pub";
}

const char *MfxProxyHostBroker::sub_address() const {
    return "inproc://plugin_host_sub";
}

void MfxProxyHostBroker::broker_thread_main() {
    {
        LOG << "[MfxProxyHost] Hello from broker thread";
    }

    // send RemoteHostStarted
    {
        flatbuffers::FlatBufferBuilder fbb;

        auto plugins = std::vector<flatbuffers::Offset<OpenMfxRemoteProxy::OfxPlugin>>();
        auto& library = host().library();

        for (int i = 0; i < library.effectCount(); i++) {
            auto plugin = OpenMfxRemoteProxy::CreateOfxPluginDirect(fbb,
                                                                    i,
                                                                    library.effectIdentifier(i),
                                                                    (int)library.effectVersionMajor(i),
                                                                    (int)library.effectVersionMinor(i));
            plugins.push_back(plugin);
        }

        auto message = OpenMfxRemoteProxy::CreateRemoteHostStartedDirect(fbb, (int)plugins.size(), &plugins);

        auto message_ = MfxProxyMessage::make_message(fbb, message, 0, 0, 0);
        send_message_pair(std::move(message_));
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

            // TODO handle the message
        }
    }
}

void MfxProxyHostBroker::setup_sockets(const char *pair_address) {
    m_pair_address = pair_address;

    m_pub_socket.bind(pub_address());
    LOG << "[" << broker_name() << "] PUB socket bind to " << m_pub_socket.get(zmq::sockopt::last_endpoint);

    m_sub_socket.bind(sub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, "");
    LOG << "[" << broker_name() << "] SUB socket bind to " << m_sub_socket.get(zmq::sockopt::last_endpoint);

    m_pair_socket.connect(pair_address);
    LOG << "[" << broker_name() << "] PAIR socket connected to " << pair_address;
}

void MfxProxyHostBroker::set_host(MfxProxyHost *host) {
    m_host = host;
}

MfxProxyHost &MfxProxyHostBroker::host() {
    return *m_host;
}

constexpr const char *MfxProxyHostBroker::broker_name() const {
    return "MfxProxyHostBroker";
}
