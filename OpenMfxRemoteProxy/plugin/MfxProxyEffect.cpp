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

    // TODO send ActionCreateInstanceStart
    // TODO receive ActionCreateInstanceEnd

    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::DestroyInstance(OfxMeshEffectHandle instance) {
    // TODO send ActionDestroyInstanceStart
    // TODO receive ActionDestroyInstanceEnd
    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::Cook(OfxMeshEffectHandle instance) {
    // TODO implement
    return kOfxStatOK;
}

OfxStatus MfxProxyEffect::IsIdentity(OfxMeshEffectHandle instance) {
    // TODO implement
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
