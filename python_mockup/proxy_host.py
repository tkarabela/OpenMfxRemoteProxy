import sys
from time import sleep
from threading import Lock, Thread

import zmq

from proxy_lib import ProxyHostBroker, ProxyPluginInstance, Message

BROKER_PUB_ADDRESS = "inproc://host_broker_pub"
BROKER_SUB_ADDRESS = "inproc://host_broker_sub"

class ProxyHost:
    def __init__(self, context: zmq.Context, plugin_broker_pair_address: str):
        self.context = context
        self.proxy_broker = ProxyHostBroker(
            pub_socket=context.socket(zmq.PUB),
            sub_socket=context.socket(zmq.SUB),
            pair_socket=context.socket(zmq.PAIR),
        )
        self.proxy_broker.pub_socket.bind(BROKER_PUB_ADDRESS)
        self.proxy_broker.sub_socket.bind(BROKER_SUB_ADDRESS)
        self.proxy_broker.sub_socket.subscribe("")
        self.proxy_broker.pair_socket.connect(plugin_broker_pair_address)

        self.proxy_effect_instances = {}

        proxy_host_global_instance = ProxyPluginInstance(
            instance_id=0,
            pub_socket=context.socket(zmq.PUB),
            sub_socket=context.socket(zmq.SUB),
            mutex=Lock(),
            is_proxy_plugin=False
        )

        self.proxy_effect_instances[proxy_host_global_instance.instance_id] = proxy_host_global_instance

        proxy_host_global_instance.pub_socket.connect(BROKER_SUB_ADDRESS)
        proxy_host_global_instance.sub_socket.connect(BROKER_PUB_ADDRESS)
        proxy_host_global_instance.sub_socket.subscribe(
            Message.get_instance_prefix(proxy_host_global_instance.instance_id))

        proxy_host_global_instance.thread = Thread(target=proxy_host_global_instance.run_proxy_thread)
        proxy_host_global_instance.thread.start()

    def run(self):
        # print("XXX lazy proxy host, sleeping for a long time...")
        # sleep(10)
        self.proxy_broker.run_proxy_thread()


def main():
    plugin_broker_pair_address = sys.argv[1]
    context = zmq.Context()
    proxy_host = ProxyHost(context, plugin_broker_pair_address)
    proxy_host.run()


if __name__ == "__main__":
    main()
