#include "MfxProxyBroker.h"

MfxProxyBroker::MfxProxyBroker()
: m_ctx(),
  m_sub_socket(m_ctx, zmq::socket_type::sub),
  m_pub_socket(m_ctx, zmq::socket_type::pub),
  m_pair_socket(m_ctx, zmq::socket_type::pair) {
}

void MfxProxyBroker::start_thread() {
    m_thread = std::thread(&MfxProxyBroker::broker_thread_main, this);
}
