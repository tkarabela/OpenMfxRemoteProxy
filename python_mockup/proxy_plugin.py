import subprocess
from threading import Thread, Lock, Condition
import zmq

from proxy_lib import ProxyPluginBroker, ProxyPluginInstance, Message

BROKER_PAIR_ADDRESS = "tcp://127.0.0.1"
BROKER_PUB_ADDRESS = "inproc://plugin_broker_pub"
BROKER_SUB_ADDRESS = "inproc://plugin_broker_sub"

class ProxyPlugin:
    def __init__(self, context: zmq.Context):
        self.context = context
        self.proxy_broker = ProxyPluginBroker(
            pub_socket=context.socket(zmq.PUB),
            sub_socket=context.socket(zmq.SUB),
            pair_socket=context.socket(zmq.PAIR),
            status_ready=None,
            status_ready_condvar=Condition()
        )
        self.proxy_broker.pub_socket.bind(BROKER_PUB_ADDRESS)
        self.proxy_broker.sub_socket.bind(BROKER_SUB_ADDRESS)
        self.proxy_broker.sub_socket.subscribe("")
        port = self.proxy_broker.pair_socket.bind_to_random_port(BROKER_PAIR_ADDRESS)

        self.pair_address = f"{BROKER_PAIR_ADDRESS}:{port}"

        self.proxy_effect_instances = {}

        global_instance = ProxyPluginInstance(
            instance_id=0,
            pub_socket=context.socket(zmq.PUB),
            sub_socket=context.socket(zmq.SUB),
            mutex=Lock(),
            is_proxy_plugin=True
        )

        self.proxy_effect_instances[global_instance.instance_id] = global_instance

        global_instance.pub_socket.connect(BROKER_SUB_ADDRESS)
        global_instance.sub_socket.connect(BROKER_PUB_ADDRESS)
        global_instance.sub_socket.subscribe(Message.get_instance_prefix(global_instance.instance_id))

    def run(self):
        self.proxy_broker.run_proxy_thread()

    def wait_for_status_ready(self):
        return self.proxy_broker.wait_for_status_ready()

    def action_describe(self, parameters: str) -> str:
        global_instance_id = 0
        instance = self.proxy_effect_instances[global_instance_id]
        return instance.action_describe(parameters)

def main():
    context = zmq.Context()
    proxy_plugin = ProxyPlugin(context)
    pair_address = proxy_plugin.pair_address

    plugin_broker_thread = Thread(target=proxy_plugin.run)
    plugin_broker_thread.start()

    cmd = ["python", "proxy_host.py", pair_address]
    print("Running command:", cmd)
    p = subprocess.Popen(" ".join(cmd), shell=True)

    # ---------------------
    print("Waiting for status ready")
    status = proxy_plugin.wait_for_status_ready()
    if not status:
        print("Status is not OK, giving up")
        # XXX kill remote process
        return

    # test OFX host calling in serial
    print("OFX host: invoking describe action...")
    rv = proxy_plugin.action_describe("describe params")  # host thread
    print("OFX host: describe action returned", rv)


if __name__ == "__main__":
    main()
