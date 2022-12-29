#include "MfxProxyEffect.h"
#include "OpenMfx/Sdk/Cpp/Common"
#include "MfxFlatbuffersMessages_generated.h"
#include "MfxFlatbuffersBasicTypes_generated.h"
#include <cassert>

MfxProxyEffect::MfxProxyEffect(int effect_id, MfxProxyPluginBroker *broker)
: m_effect_id(effect_id), m_broker(broker) {
}

OfxStatus MfxProxyEffect::Load() {
    {
        DEBUG_LOG << "[MfxProxyEffect] Effect " << GetName() << " connects to m_broker";
    }

    m_pub_socket = zmq::socket_t(broker().ctx(), zmq::socket_type::pub);
    m_sub_socket = zmq::socket_t(broker().ctx(), zmq::socket_type::sub);

    m_pub_socket.connect(broker().sub_address());
    m_sub_socket.connect(broker().pub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, message_prefix());

    // Since these sockets are using `inproc` transport, we assume they
    // connect instantaneously, and there is no confirmation mechanism
    // like with `RemoteHostStarted`.

    auto message_thread_id = broker().generate_message_thread_id();

    // send ActionLoadStart
    {
        flatbuffers::FlatBufferBuilder fbb;
        auto message = OpenMfxRemoteProxy::CreateActionLoadStart(fbb);
        auto message_envelope = OpenMfxRemoteProxy::CreateMessageEnvelope(
                fbb,
                message_thread_id,
                OpenMfxRemoteProxy::Message::Message_ActionLoadStart,
                message.Union());

        fbb.Finish(message_envelope);
        send_flatbuffer(fbb, message_thread_id);
    }

    // receive ActionLoadEnd
    auto msg = receive_flatbuffer(message_thread_id);
    auto action = msg.message_envelope()->message_as_ActionLoadEnd();
    assert(action); // check message type

    return action->status()->status(); // return remote status
}

OfxStatus MfxProxyEffect::Unload() {
    auto message_thread_id = broker().generate_message_thread_id();

    // send ActionUnloadStart
    {
        flatbuffers::FlatBufferBuilder fbb;
        auto message = OpenMfxRemoteProxy::CreateActionUnloadStart(fbb);
        auto message_envelope = OpenMfxRemoteProxy::CreateMessageEnvelope(
                fbb,
                message_thread_id,
                OpenMfxRemoteProxy::Message::Message_ActionUnloadStart,
                message.Union());

        fbb.Finish(message_envelope);
        send_flatbuffer(fbb, message_thread_id);
    }

    // receive ActionUnloadEnd
    auto msg = receive_flatbuffer(message_thread_id);
    auto action = msg.message_envelope()->message_as_ActionUnloadEnd();
    assert(action); // check message type

    {
        DEBUG_LOG << "[MfxProxyEffect] Effect " << GetName() << " disconnects from m_broker";
    }

    m_pub_socket.close();
    m_sub_socket.close();

    return action->status()->status(); // return remote status
}

OfxStatus MfxProxyEffect::Describe(OfxMeshEffectHandle descriptor) {
    auto message_thread_id = broker().generate_message_thread_id();

    // send ActionDescribeStart
    {
        flatbuffers::FlatBufferBuilder fbb;
        auto message = OpenMfxRemoteProxy::CreateActionDescribeStart(fbb);
        auto message_envelope = OpenMfxRemoteProxy::CreateMessageEnvelope(
                fbb,
                message_thread_id,
                OpenMfxRemoteProxy::Message::Message_ActionDescribeStart,
                message.Union());

        fbb.Finish(message_envelope);
        send_flatbuffer(fbb, message_thread_id);
    }

    // receive ActionDescribeEnd
    auto msg = receive_flatbuffer(message_thread_id);
    auto action = msg.message_envelope()->message_as_ActionDescribeEnd();
    assert(action); // check message type
    MFX_ENSURE(action->status()->status()); // check that remote status was OK

    // replay received parameters
    OfxParamSetHandle ofx_parameters;
    meshEffectSuite->getParamSet(descriptor, &ofx_parameters);

    for (auto parameter : *action->parameters()) {
        const char *paramName = nullptr, *paramType = nullptr;
        for (auto property : *parameter->properties()) {
            if (property->name()->string_view() == kOfxPropName) {
                paramName = property->name()->c_str();
            } else if (property->name()->string_view() == kOfxParamPropType) {
                paramType = property->name()->c_str();
            }
        }
        assert(paramName);
        assert(paramType);

        OfxPropertySetHandle ofx_param_props;
        parameterSuite->paramDefine(ofx_parameters, paramType, paramName, &ofx_param_props);

        for (auto property : *parameter->properties()) {
            if (property->name()->string_view() == kOfxPropName ||
                property->name()->string_view() == kOfxParamPropType) {
                // do nothing, already set by paramDefine
            } else {
                MFX_ENSURE(prop_set_from_flatbuffer(ofx_param_props, property));
            }
        }
    }

    // replay received inputs
    for (auto input : *action->inputs()) {
        const char *inputName = nullptr;
        for (auto property : *input->properties()) {
            if (property->name()->string_view() == kOfxPropName) {
                inputName = property->name()->c_str();
            }
        }
        assert(inputName);

        OfxPropertySetHandle ofx_input_props;
        meshEffectSuite->inputDefine(descriptor, inputName, nullptr, &ofx_input_props);

        // TODO when RFC007 (requesting attributes) is implemented, it will need to be replayed here
        if (input->attributes()->size() > 0) {
            WARN_LOG << "[MfxProxyEffect] Received some attribute definitions in Describe, ignoring";
        }

        for (auto property : *input->properties()) {
            if (property->name()->string_view() == kOfxPropName) {
                // do nothing, already set by inputDefine
            } else {
                MFX_ENSURE(prop_set_from_flatbuffer(ofx_input_props, property));
            }
        }
    }

    return action->status()->status(); // return remote status
}

OfxStatus MfxProxyEffect::CreateInstance(OfxMeshEffectHandle instance) {
//    uint32_t instance_id = m_last_instance_id++;
//    MFX_ENSURE(set_instance_data(instance, new InstanceData { instance_id }));
    // XXX assign ids on proxy host side, on success

    // TODO send ActionCreateInstanceStart
    // TODO receive ActionCreateInstanceEnd

    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::DestroyInstance(OfxMeshEffectHandle instance) {
    MFX_ENSURE(delete_instance_data(instance));

    // TODO send ActionDestroyInstanceStart
    // TODO receive ActionDestroyInstanceEnd
    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::Cook(OfxMeshEffectHandle instance) {
    // TODO implement
    auto instance_id = get_instance_data(instance)->instance_id;

    return kOfxStatFailed;
}

OfxStatus MfxProxyEffect::IsIdentity(OfxMeshEffectHandle instance) {
    // TODO implement
    auto instance_id = get_instance_data(instance)->instance_id;

    return kOfxStatReplyDefault;
}

OfxStatus MfxProxyEffect::MainEntry(const char *action,
                                    const void *handle,
                                    OfxPropertySetHandle inArgs,
                                    OfxPropertySetHandle outArgs) {
    OfxStatus status = kOfxStatErrUnknown;

    {
        DEBUG_LOG << "[MfxProxyEffect] Acquiring lock for effect " << GetName() << ", action " << action << ", handle " << handle;
    }
    {
        std::lock_guard<std::mutex> guard(m_effect_lock);
        {
            DEBUG_LOG << "[MfxProxyEffect] Got lock for effect " << GetName() << ", action " << action << ", handle " << handle;
        }
        try {
            status = MfxEffect::MainEntry(action, handle, inArgs, outArgs);
        } catch (zmq::error_t &err) {
            ERR_LOG << "[MfxProxyEffect] ZMQ error: " << err.what();
            return kOfxStatFailed;
        }
    }
    {
        DEBUG_LOG << "[MfxProxyEffect] Released lock for effect " << GetName() << ", action " << action << ", handle " << handle;
    }

    return status;
}

MfxProxyPluginBroker& MfxProxyEffect::broker() {
    return *m_broker;
}

zmq::const_buffer MfxProxyEffect::message_prefix() const {
    return zmq::const_buffer { (void*)&m_effect_id, sizeof(m_effect_id) };
}

MfxProxyEffect::InstanceData *MfxProxyEffect::get_instance_data(OfxMeshEffectHandle instance) {
    OfxPropertySetHandle instance_props;
    meshEffectSuite->getPropertySet(instance, &instance_props);
    InstanceData *data = nullptr;
    propertySuite->propGetPointer(instance_props, kOfxPropInstanceData, 0, (void**)&data);
    return data;
}

OfxStatus MfxProxyEffect::set_instance_data(OfxMeshEffectHandle instance, MfxProxyEffect::InstanceData *data) {
    OfxPropertySetHandle instance_props;
    MFX_ENSURE(meshEffectSuite->getPropertySet(instance, &instance_props));
    MFX_ENSURE(propertySuite->propSetPointer(instance_props, kOfxPropInstanceData, 0, data));
    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::delete_instance_data(OfxMeshEffectHandle instance) {
    OfxPropertySetHandle instance_props;
    MFX_ENSURE(meshEffectSuite->getPropertySet(instance, &instance_props));
    InstanceData *instance_data = nullptr;
    MFX_ENSURE(propertySuite->propGetPointer(instance_props, kOfxPropInstanceData, 0, (void**)&instance_data));
    delete instance_data;
    return kOfxStatOK;
}

void MfxProxyEffect::send_flatbuffer(flatbuffers::FlatBufferBuilder &fbb, uint64_t message_thread_id) {
    uint8_t *buf = fbb.GetBufferPointer();
    auto bufsize = fbb.GetSize();
    assert(bufsize > 0);

    // we copy the data here, which is suboptimal

    zmq::message_t msg(sizeof(m_effect_id) + bufsize);
    memcpy(msg.data(),
           &m_effect_id,
           sizeof(m_effect_id));
    memcpy(msg.data<uint8_t>() + sizeof(m_effect_id),
           buf,
           bufsize);

    DEBUG_LOG << "[MfxProxyEffect] Message thread " << message_thread_id << " sends message";

    auto res = m_pub_socket.send(std::move(msg), zmq::send_flags::none);
}

FlatbufferRecvMessage MfxProxyEffect::receive_flatbuffer(uint64_t message_thread_id) {
    zmq::message_t msg;
    auto res = m_sub_socket.recv(msg);

    FlatbufferRecvMessage msg_(std::move(msg));

    DEBUG_LOG << "[MfxProxyEffect] Message thread " << msg_.message_envelope()->message_thread_id() << " got message " << EnumNameMessage(msg_.message_envelope()->message_type());

    assert(msg_.message_effect_id() == m_effect_id);
    assert(msg_.message_envelope()->message_thread_id() == message_thread_id);

    return msg_;
}

OfxStatus MfxProxyEffect::prop_set_from_flatbuffer(OfxPropertySetHandle ofx_param_props,
                                                   const OpenMfxRemoteProxy::OfxProperty *property) {
    auto propName = property->name()->c_str();

    switch (property->value_type()) {
        case OpenMfxRemoteProxy::OfxPropertyValue_OfxPropertyIntNValue:
        {
            auto value = property->value_as_OfxPropertyIntNValue();
            for (int i = 0; i < value->values()->size(); i++) {
                MFX_ENSURE(propertySuite->propSetInt(ofx_param_props, propName, i, value->values()->Get(i)));
            }
        }
            break;
        case OpenMfxRemoteProxy::OfxPropertyValue_OfxPropertyFloatNValue:
        {
            auto value = property->value_as_OfxPropertyFloatNValue();
            for (int i = 0; i < value->values()->size(); i++) {
                MFX_ENSURE(propertySuite->propSetDouble(ofx_param_props, propName, i, value->values()->Get(i)));
            }
        }
            break;
        case OpenMfxRemoteProxy::OfxPropertyValue_OfxPropertyStringNValue:
        {
            auto value = property->value_as_OfxPropertyStringNValue();
            for (int i = 0; i < value->values()->size(); i++) {
                MFX_ENSURE(propertySuite->propSetString(ofx_param_props, propName, i, value->values()->Get(i)->c_str()));
            }
        }
            break;
        default:
        {
            WARN_LOG << "[MfxProxyEffect] Ignoring property " << property->name()->string_view();
        }
            break;
    }

    return kOfxStatOK;
}

MfxProxyEffect::MfxProxyEffect(MfxProxyEffect &&other) noexcept
: m_effect_id(other.m_effect_id),
  m_pub_socket(std::move(other.m_pub_socket)),
  m_sub_socket(std::move(other.m_sub_socket)),
  m_effect_lock(),
  m_broker(other.m_broker) {
}

const char *MfxProxyEffect::GetName() {
    return m_identifier.c_str();
}

void MfxProxyEffect::SetName(const char *identifier) {
    m_identifier = std::string(identifier);
}
