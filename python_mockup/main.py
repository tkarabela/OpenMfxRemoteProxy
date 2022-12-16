from dataclasses import dataclass
from time import sleep

import zmq
from threading import Lock, Thread

PRINT_LOCK = Lock()
def print_(*args):
    with PRINT_LOCK:
        print(*args)

@dataclass
class ProxyBroker:
    """
    ProxyBroker

    Attributes:
        pub_socket: push requests/responses to instances
        sub_socket: receive requests/responses from instances
        pair_socket: IPC communication with the other broker
    """
    pub_socket: zmq.Socket
    sub_socket: zmq.Socket
    pair_socket: zmq.Socket
    is_proxy_plugin: bool

    def run_proxy_thread(self):
        print_(f"{self}: Hello")

        rlist_ = [self.sub_socket, self.pair_socket]
        wlist_ = []  # XXX ?
        xlist_ = [self.pub_socket, self.sub_socket, self.pair_socket]

        # XXX here we really need to poll
        while True:
            print_(f"{self}: Calling zmq.select")
            rlist, wlist, xlist = zmq.select(rlist_, wlist_, xlist_)
            for sock in rlist:
                data = sock.recv()
                msg = Message.from_bytes(data)
                print_(f"{self}: Received message: {msg}")

            for sock in xlist:
                print_(f"{self}: Error on socket: {sock}")

    def __str__(self) -> str:
        if self.is_proxy_plugin:
            return "ProxyBroker(plugin)"
        else:
            return "ProxyBroker(host)"

@dataclass
class ProxyPluginInstance:
    instance_id: int
    pub_socket: zmq.Socket
    sub_socket: zmq.Socket
    mutex: Lock
    is_proxy_plugin: bool

    def make_request_message(self, body: str) -> "Message":
        return Message(self.instance_id, False, body)

    def action_describe(self, parameters: str):
        print_(f"{self}: starting action describe with parameters: {parameters}")
        if self.is_proxy_plugin:
            return self._plugin_action_describe(parameters)
        else:
            return self._host_action_describe(parameters)

    def _plugin_action_describe(self, parameters: str):
        print_(f"{self}: acquiring mutex")
        with self.mutex:
            cook_msg = self.make_request_message(f"cook({parameters})")
            print_(f"{self}: sending message to pub socket: {cook_msg}")
            self.pub_socket.send(cook_msg.to_bytes())
            print_(f"{self}: after pub_socket.send()")

            while True:
                print_(f"{self}: calling sub_socket.recv()")
                data = self.sub_socket.recv()
                print_(f"{self}: received data: {data}")

    def _host_action_describe(self, parameters: str):
        raise NotImplementedError

    def run_proxy_thread(self):
        print_(f"{self}: Hello")
        assert not self.is_proxy_plugin, "host should use host threads"

        rlist_ = [self.sub_socket]
        wlist_ = []  # XXX ?
        xlist_ = [self.pub_socket, self.sub_socket]

        while True:
            # XXX just recv() here?
            rlist, wlist, xlist = zmq.select(rlist_, wlist_, xlist_)
            for sock in rlist:
                data = sock.recv()
                msg = Message.from_bytes(data)
                print_(f"{self}: Received message: {msg}")

            for sock in xlist:
                print_(f"{self}: Error on socket: {sock}")

    def __str__(self) -> str:
        if self.is_proxy_plugin:
            return f"ProxyPluginInstance(plugin, id={self.instance_id})"
        else:
            return f"ProxyPluginInstance(host, id={self.instance_id})"

@dataclass
class ProxyPlugin:
    broker: ProxyBroker
    effect_instances: list[ProxyPluginInstance]

@dataclass
class ProxyHost:
    broker: ProxyBroker
    effect_instances: list[ProxyPluginInstance]


@dataclass
class Message:
    instance_id: int
    response: bool
    body: str

    def make_reply(self, body: str) -> "Message":
        return Message(self.instance_id, True, body)

    @staticmethod
    def get_instance_prefix(instance_id) -> bytes:
        return f"{instance_id:08d}".encode("ascii")

    def to_bytes(self) -> bytes:
        return f"{self.instance_id:08d}-{self.response:d}-{self.body}".encode("ascii")

    @classmethod
    def from_bytes(cls, s: bytes) -> "Message":
        return cls(
            instance_id=int(s[0:8].decode("ascii")),
            response=s[9] == b"1",
            body=s[11:].decode("ascii")
        )

def main():
    context = zmq.Context()

    plugin_broker_pub_address = "tcp://127.0.0.1:5555"
    plugin_broker_sub_address = "tcp://127.0.0.1:5556"
    plugin_broker_pair_address = "tcp://127.0.0.1:5557"
    host_broker_pub_address = "tcp://127.0.0.1:5558"
    host_broker_sub_address = "tcp://127.0.0.1:5559"
    # proxy_plugin_global_instance_pub_address = "tcp://127.0.0.1:5560"  # XXX dont want this, broker should bind
    # proxy_plugin_global_instance_sub_address = "tcp://127.0.0.1:5561"
    # proxy_host_global_instance_pub_address = "tcp://127.0.0.1:5562"
    # proxy_host_global_instance_sub_address = "tcp://127.0.0.1:5563"

    plugin_broker = ProxyBroker(
        pub_socket=context.socket(zmq.PUB),
        sub_socket=context.socket(zmq.SUB),
        pair_socket=context.socket(zmq.PAIR),
        is_proxy_plugin=True
    )

    host_broker = ProxyBroker(
        pub_socket=context.socket(zmq.PUB),
        sub_socket=context.socket(zmq.SUB),
        pair_socket=context.socket(zmq.PAIR),
        is_proxy_plugin=False
    )

    plugin_broker.pub_socket.bind(plugin_broker_pub_address)
    plugin_broker.sub_socket.bind(plugin_broker_sub_address)
    plugin_broker.sub_socket.subscribe("")
    plugin_broker.pair_socket.bind(plugin_broker_pair_address)
    host_broker.pub_socket.bind(host_broker_pub_address)
    host_broker.sub_socket.bind(host_broker_sub_address)
    host_broker.sub_socket.subscribe("")
    host_broker.pair_socket.connect(plugin_broker_pair_address)

    proxy_plugin_global_instance = ProxyPluginInstance(
        instance_id=0,
        pub_socket=context.socket(zmq.PUB),
        sub_socket=context.socket(zmq.SUB),
        mutex=Lock(),
        is_proxy_plugin=True
    )

    proxy_plugin_global_instance.pub_socket.connect(plugin_broker_sub_address)
    proxy_plugin_global_instance.sub_socket.connect(plugin_broker_pub_address)
    proxy_plugin_global_instance.sub_socket.subscribe(Message.get_instance_prefix(proxy_plugin_global_instance.instance_id))

    proxy_host_global_instance = ProxyPluginInstance(
        instance_id=0,
        pub_socket=context.socket(zmq.PUB),
        sub_socket=context.socket(zmq.SUB),
        mutex=Lock(),
        is_proxy_plugin=False
    )

    proxy_host_global_instance.pub_socket.connect(host_broker_sub_address)
    proxy_host_global_instance.sub_socket.connect(host_broker_pub_address)
    proxy_host_global_instance.sub_socket.subscribe(Message.get_instance_prefix(proxy_host_global_instance.instance_id))

    # -------------------------------------

    print_("starting plugin_broker_thread...")
    plugin_broker_thread = Thread(target=plugin_broker.run_proxy_thread)
    plugin_broker_thread.start()

    print_("starting host_broker_thread...")
    host_broker_thread = Thread(target=host_broker.run_proxy_thread)
    host_broker_thread.start()

    print_("starting proxy_host_global_instance_thread...")
    proxy_host_global_instance_thread = Thread(target=proxy_host_global_instance.run_proxy_thread)
    proxy_host_global_instance_thread.start()

    # -------------------------------------
    # XXX issue: message sent before receiver is ready
    print_("XXX sleeping to make sure everything starts")
    sleep(1)

    print_("OFX host: invoking describe action...")
    rv = proxy_plugin_global_instance.action_describe("describe params")  # host thread
    print_("OFX host: describe action returned", rv)


main()
