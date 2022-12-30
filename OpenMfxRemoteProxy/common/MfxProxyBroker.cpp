#include "MfxProxyBroker.h"
#include "OpenMfx/Sdk/Cpp/Common"

MfxProxyBroker::MfxProxyBroker()
: m_ctx(),
  m_sub_socket(m_ctx, zmq::socket_type::sub),
  m_pub_socket(m_ctx, zmq::socket_type::pub),
  m_pair_socket(m_ctx, zmq::socket_type::pair) {
}

void MfxProxyBroker::start_thread() {
    m_thread = std::thread(&MfxProxyBroker::broker_thread_main, this);
}

void MfxProxyBroker::join_thread() {
    m_thread.join();
}

void MfxProxyBroker::send_message_pub(MfxProxyMessage &&message) {
    DEBUG_LOG << "[" << broker_name() << "]"
              << " Sending message " << OpenMfxRemoteProxy::EnumNameMessage(message.message_type()) << " to PUB:"
              << " message_id=" << message.message_id()
              << " message_thread_id=" << message.message_thread_id()
              << " effect_id=" << message.effect_id();

    auto res = m_pub_socket.send(message.move_msg(), zmq::send_flags::none);
}

void MfxProxyBroker::send_message_pair(MfxProxyMessage &&message) {
    DEBUG_LOG << "[" << broker_name() << "]"
              << " Sending message " << OpenMfxRemoteProxy::EnumNameMessage(message.message_type()) << " to PAIR:"
              << " message_id=" << message.message_id()
              << " message_thread_id=" << message.message_thread_id()
              << " effect_id=" << message.effect_id();

    auto res = m_pair_socket.send(message.move_msg(), zmq::send_flags::none);
}
