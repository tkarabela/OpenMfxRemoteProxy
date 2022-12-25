#include "MfxProxyEffect.h"
#include "OpenMfx/Sdk/Cpp/Common"

MfxProxyEffect::MfxProxyEffect(int effect_id)
: m_effect_id(effect_id),
  m_last_instance_id(0) {
}

OfxStatus MfxProxyEffect::Load() {
    m_pub_socket = zmq::socket_t(broker().ctx(), zmq::socket_type::pub);
    m_sub_socket = zmq::socket_t(broker().ctx(), zmq::socket_type::sub);

    m_pub_socket.connect(broker().sub_address());
    m_sub_socket.connect(broker().pub_address());
    m_sub_socket.set(zmq::sockopt::subscribe, message_prefix());

    // TODO send ActionLoadStart
    // TODO receive ActionLoadEnd

    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::Unload() {
    // TODO send ActionUnloadStart
    // TODO receive ActionUnloadEnd

    m_pub_socket.close();
    m_sub_socket.close();

    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::Describe(OfxMeshEffectHandle descriptor) {
    // TODO send ActionDescribeStart
    // TODO receive ActionDescribeEnd
    // TODO replay received parameters to the descriptor
    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::CreateInstance(OfxMeshEffectHandle instance) {
    uint32_t instance_id = m_last_instance_id++;
    MFX_ENSURE(set_instance_data(instance, new InstanceData { instance_id }));

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

    return kOfxStatOK;
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

MfxProxyBroker& MfxProxyEffect::broker() {
    return MfxProxyBroker::instance();
}

zmq::const_buffer MfxProxyEffect::message_prefix() const {
    return zmq::const_buffer { (void*)&m_effect_id, sizeof(m_effect_id) };
}

MfxProxyEffect::InstanceData *MfxProxyEffect::get_instance_data(OfxMeshEffectHandle instance) {
    OfxPropertySetHandle instance_props;
    meshEffectSuite->getPropertySet(instance, &instance_props);
    InstanceData *data = nullptr;
    propertySuite->propGetPointer(instance_props, kOfxPropInstanceData, 0, (void**)&data);
    return data
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
