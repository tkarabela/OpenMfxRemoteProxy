#pragma once

#include <zmq.hpp>
#include "MfxFlatbuffersMessages_generated.h"

/**
 * Wrapper that owns a zmq::message_t which has data consisting of:
 * - uint32_t effect_id (used for routing messages between brokers and effects)
 * - OpenMfxRemoteProxy::MessageEnvelope (root flatbuffer type)
 * */
class MfxProxyMessage {
public:
    explicit MfxProxyMessage(zmq::message_t&& msg);
    MfxProxyMessage(MfxProxyMessage&& msg) noexcept;
    MfxProxyMessage(const MfxProxyMessage&) = delete;
    MfxProxyMessage& operator=(const MfxProxyMessage&) = delete;

    template <typename T>
    static MfxProxyMessage
    make_message(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<T> &message, uint32_t effect_id,
                 uint32_t message_thread_id, uint32_t message_id);

    uint32_t effect_id();
    uint32_t message_id();
    uint32_t message_thread_id();
    const OpenMfxRemoteProxy::MessageEnvelope* message_envelope();
    OpenMfxRemoteProxy::Message message_type();

    zmq::message_t&& move_msg();

    template <typename T>
    const T* message_as();

protected:
    zmq::message_t m_msg;
};

template<typename T>
MfxProxyMessage
MfxProxyMessage::make_message(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<T> &message, uint32_t effect_id,
                              uint32_t message_thread_id, uint32_t message_id) {
    static_assert(OpenMfxRemoteProxy::MessageTraits<T>::enum_value != OpenMfxRemoteProxy::Message_NONE);

    auto message_envelope = OpenMfxRemoteProxy::CreateMessageEnvelope(
            fbb,
            message_thread_id,
            message_id,
            OpenMfxRemoteProxy::MessageTraits<T>::enum_value,
            message.Union());

    fbb.Finish(message_envelope);

    uint8_t *buf = fbb.GetBufferPointer();
    auto bufsize = fbb.GetSize();
    assert(bufsize > 0);

    // we copy the data here, which is suboptimal

    zmq::message_t msg(sizeof(effect_id) + bufsize);
    memcpy(msg.data(),
           &effect_id,
           sizeof(effect_id));
    memcpy(msg.data<uint8_t>() + sizeof(effect_id),
           buf,
           bufsize);

    return MfxProxyMessage(std::move(msg));
}

template<typename T>
const T *MfxProxyMessage::message_as() {
    auto ptr = message_envelope()->message_as<T>();
    if (!ptr) {
        throw std::runtime_error("MfxProxyMessage type mismatch");
    }
    return ptr;
}
