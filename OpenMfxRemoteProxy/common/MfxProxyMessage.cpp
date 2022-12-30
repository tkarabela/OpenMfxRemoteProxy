#include "MfxProxyMessage.h"

MfxProxyMessage::MfxProxyMessage(zmq::message_t&& msg)
: m_msg(std::move(msg)) {
}

MfxProxyMessage::MfxProxyMessage(MfxProxyMessage &&msg) noexcept
: m_msg(std::move(msg.m_msg)) {
}

const OpenMfxRemoteProxy::MessageEnvelope *MfxProxyMessage::message_envelope() {
    assert(!m_msg.empty());
    auto flatbuffer_ptr = m_msg.data<uint8_t>() + sizeof(uint32_t);
    return OpenMfxRemoteProxy::GetMessageEnvelope(flatbuffer_ptr);
}

uint32_t MfxProxyMessage::effect_id() {
    assert(!m_msg.empty());
    return m_msg.data<uint32_t>()[0];
}

uint32_t MfxProxyMessage::message_id() {
    return message_envelope()->message_id();
}

uint32_t MfxProxyMessage::message_thread_id() {
    return message_envelope()->message_thread_id();
}

zmq::message_t &&MfxProxyMessage::move_msg() {
    return std::move(m_msg);
}

OpenMfxRemoteProxy::Message MfxProxyMessage::message_type() {
    return message_envelope()->message_type();
}
