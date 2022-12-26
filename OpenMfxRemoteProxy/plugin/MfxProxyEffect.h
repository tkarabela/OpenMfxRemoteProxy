#pragma once

#include <mutex>
#include "OpenMfx/Sdk/Cpp/Plugin/MfxEffect"
#include "MfxProxyBroker.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "MfxFlatbuffersMessages_generated.h"

class FlatbufferRecvMessage {
public:
    explicit FlatbufferRecvMessage(zmq::message_t&& msg_) : msg(std::move(msg_)) {}

    uint32_t message_effect_id() {
        return msg.data<uint32_t>()[0];
    }

    const OpenMfxRemoteProxy::MessageEnvelope* message_envelope() {
        auto flatbuffer_ptr = msg.data<uint8_t>() + sizeof(uint32_t);
        return OpenMfxRemoteProxy::GetMessageEnvelope(flatbuffer_ptr);
    }
private:
    zmq::message_t msg;
};

class MfxProxyEffect : public MfxEffect {
public:
    explicit MfxProxyEffect(int effect_id);

    /**
     * A locking version of MfxEffect::MainEntry() that locks on m_effect_lock.
     * This is necessary since the SDK is probably not thread-safe due to SetupDescribe/SetupCook/SetupIsIdentity,
     * and it also simplifies communication with remote proxy host (since there can be only one action "in flight"
     * at any given time).
     * It's suboptimal since it prevents the host from eg. cooking different effect instances in parallel,
     * or different timesteps of one effect instance.
     *
     * It also catches ZMQ exceptions and prints them.
     * */
    OfxStatus MainEntry(const char *action,
                        const void *handle,
                        OfxPropertySetHandle inArgs,
                        OfxPropertySetHandle outArgs);

    /**
     * This is stored as kOfxPropInstanceData.
     */
    struct InstanceData {
        uint32_t instance_id;
    };

protected:
    OfxStatus Load() override;
    OfxStatus Unload() override;
    OfxStatus Describe(OfxMeshEffectHandle descriptor) override;
    OfxStatus CreateInstance(OfxMeshEffectHandle instance) override;
    OfxStatus DestroyInstance(OfxMeshEffectHandle instance) override;
    OfxStatus Cook(OfxMeshEffectHandle instance) override;
    OfxStatus IsIdentity(OfxMeshEffectHandle instance) override;

    static MfxProxyBroker& broker();
    zmq::const_buffer message_prefix() const;

    InstanceData* get_instance_data(OfxMeshEffectHandle instance);
    OfxStatus set_instance_data(OfxMeshEffectHandle instance, InstanceData *data);
    OfxStatus delete_instance_data(OfxMeshEffectHandle instance);

    void send_flatbuffer(flatbuffers::FlatBufferBuilder &fbb, uint64_t message_thread_id);
    FlatbufferRecvMessage receive_flatbuffer(uint64_t message_thread_id);

    OfxStatus prop_set_from_flatbuffer(OfxPropertySetHandle ofx_param_props, const OpenMfxRemoteProxy::OfxProperty* property);

protected:
    uint32_t m_effect_id;
    zmq::socket_t m_pub_socket;
    zmq::socket_t m_sub_socket;
    std::mutex m_effect_lock;
};
