#include "MfxProxyHostBroker.h"
#include "MfxProxyHost.h"
#include "OpenMfx/Sdk/Cpp/Common"
#include "flatbuffers/flatbuffer_builder.h"
#include "MfxFlatbuffersMessages_generated.h"
#include "MfxFlatbuffersBasicTypes_generated.h"
#include "zmq.hpp"
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
        uint64_t message_thread_id = 0;
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
        auto message_envelope = OpenMfxRemoteProxy::CreateMessageEnvelope(
                fbb,
                message_thread_id,
                OpenMfxRemoteProxy::Message::Message_RemoteHostStarted,
                message.Union());

        fbb.Finish(message_envelope);
        {
            DEBUG_LOG << "[MfxProxyHost] Sending RemoteHostStarted message";
        }
        send_flatbuffer_remote(fbb, message_thread_id);
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
            DEBUG_LOG << "[MfxProxyHost] Broker got message from socket " << broker_socket_to_string(*events[i].user_data);
            zmq::message_t msg;
            auto rres = events[i].socket.recv(msg, zmq::recv_flags::none);
            // TODO handle the message
        }
    }
}

void MfxProxyHostBroker::setup_sockets(const char *pair_address) {
    m_pair_address = pair_address;

    m_pub_socket.bind(pub_address());
    LOG << "[MfxProxyHost] Broker PUB socket bind to " << m_pub_socket.get(zmq::sockopt::last_endpoint);

    m_sub_socket.bind(sub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, "");
    LOG << "[MfxProxyHost] Broker SUB socket bind to " << m_sub_socket.get(zmq::sockopt::last_endpoint);

    m_pair_socket.connect(pair_address);
    LOG << "[MfxProxyHost] Broker PAIR socket connected to " << pair_address;
}

void MfxProxyHostBroker::set_host(MfxProxyHost *host) {
    m_host = host;
}

MfxProxyHost &MfxProxyHostBroker::host() {
    return *m_host;
}

void MfxProxyHostBroker::send_flatbuffer_remote(flatbuffers::FlatBufferBuilder &fbb, uint64_t message_thread_id) {
    uint8_t *buf = fbb.GetBufferPointer();
    auto bufsize = fbb.GetSize();
    assert(bufsize > 0);

    // we copy the data here, which is suboptimal
    uint32_t effect_id = 0;

    zmq::message_t msg(sizeof(effect_id) + bufsize);
    memcpy(msg.data(),
           &effect_id,
           sizeof(effect_id));
    memcpy(msg.data<uint8_t>() + sizeof(effect_id),
           buf,
           bufsize);

    DEBUG_LOG << "[MfxProxyHost] Message thread " << message_thread_id << " sends message to PAIR";

    auto res = m_pair_socket.send(std::move(msg), zmq::send_flags::none);
}
