import concurrent.futures
from dataclasses import dataclass
from time import sleep
from enum import Enum
import json
from typing import Optional

import zmq
from threading import Lock, Thread, Condition

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

    def run_proxy_thread(self):
        raise NotImplementedError

@dataclass
class ProxyPluginBroker(ProxyBroker):
    status_ready: Optional[bool]
    status_ready_condvar: Condition

    def wait_for_status_ready(self) -> bool:
        with self.status_ready_condvar:
            self.status_ready_condvar.wait_for(lambda: self.status_ready is not None)
            return self.status_ready

    def run_proxy_thread(self):
        # first, wait on proxy host

        with self.status_ready_condvar:
            if self.pair_socket.poll(5000):
                data = self.pair_socket.recv()
                msg = Message.from_bytes(data)
                if msg.type_ == MessageType.RemoteHostStarted:
                    print_(f"{self} - got confirmation from remote host, starting main loop")
                    self.status_ready = True
                else:
                    self.status_ready = False
                    print_(f"{self} - expected RemoteHostStarted! got {msg}")
                self.status_ready_condvar.notify_all()
            else:
                self.status_ready = False
                print_(f"{self} - timeout while waiting for RemoteHostStarted!")
                self.status_ready_condvar.notify_all()

        # start main loop

        rlist_ = [self.sub_socket, self.pair_socket]
        wlist_ = []  # XXX ?
        xlist_ = [self.pub_socket, self.sub_socket, self.pair_socket]

        while True:
            print_(f"{self}: Calling zmq.select")
            rlist, wlist, xlist = zmq.select(rlist_, wlist_, xlist_)
            for sock in rlist:
                data = sock.recv()
                if sock is self.sub_socket:
                    print_(f"{self}: Received message from sub socket, forwarding to pair: {data}")
                    self.pair_socket.send(data)  # XXX efficiency?
                elif sock is self.pair_socket:
                    print_(f"{self}: Received message from pair socket, forwarding to pub: {data}")
                    self.pub_socket.send(data)
                else:
                    raise RuntimeError

            for sock in xlist:
                print_(f"{self}: Error on socket: {sock}")

    def __str__(self) -> str:
        return "ProxyBroker(plugin)"

class ProxyHostBroker(ProxyBroker):
    def run_proxy_thread(self):
        # first, notify proxy plugin

        print_(f"{self} - sending confirmation to remote plugin")
        msg = Message(-1, MessageType.RemoteHostStarted, {})
        self.pair_socket.send(msg.to_bytes())

        # start main loop

        rlist_ = [self.sub_socket, self.pair_socket]
        wlist_ = []  # XXX ?
        xlist_ = [self.pub_socket, self.sub_socket, self.pair_socket]

        while True:
            print_(f"{self}: Calling zmq.select")
            rlist, wlist, xlist = zmq.select(rlist_, wlist_, xlist_)
            for sock in rlist:
                data = sock.recv()
                if sock is self.sub_socket:
                    print_(f"{self}: Received message from sub socket, forwarding to pair: {data}")
                    self.pair_socket.send(data)  # XXX efficiency?
                elif sock is self.pair_socket:
                    print_(f"{self}: Received message from pair socket, forwarding to pub: {data}")
                    self.pub_socket.send(data)
                else:
                    raise RuntimeError

            for sock in xlist:
                print_(f"{self}: Error on socket: {sock}")

    def __str__(self) -> str:
        return "ProxyBroker(host)"

@dataclass
class ProxyPluginInstance:
    instance_id: int
    pub_socket: zmq.Socket
    sub_socket: zmq.Socket
    is_proxy_plugin: bool
    mutex: Optional[Lock] = None
    thread: Optional[Thread] = None

    def make_request_message(self, type_: "MessageType", data: dict) -> "Message":
        return Message(self.instance_id, type_, data)

    def action_describe(self, parameters: str) -> str:
        print_(f"{self}: starting action describe with parameters: {parameters}")
        if self.is_proxy_plugin:
            return self._plugin_action_describe(parameters)
        else:
            self._host_action_describe(parameters)
            return ""

    def _plugin_action_describe(self, parameters: str) -> str:
        print_(f"{self}: acquiring mutex")
        with self.mutex:
            cook_msg = self.make_request_message(MessageType.ActionDescribeStart, {"parameters": parameters})
            print_(f"{self}: sending message to pub socket: {cook_msg}")
            self.pub_socket.send(cook_msg.to_bytes())
            print_(f"{self}: after pub_socket.send()")

            while True:
                print_(f"{self}: calling sub_socket.recv()")
                data = self.sub_socket.recv()
                msg = Message.from_bytes(data)
                if msg.type_ == MessageType.ActionDescribeEnd:
                    inputs = msg.data["inputs"]
                    parameters = msg.data["parameters"]
                    status = msg.data["status"]
                    print_(f"{self}: now I would call OFX host and set inputs: {inputs}")
                    print_(f"{self}: now I would call OFX host and set parameters: {parameters}")
                    print_(f"{self}: returning from describe with received status: {status}")
                    return status
                else:
                    print_(f"{self}: unexpected message in describe!")
                    break

        return "kOfxStatusErr"

    def _host_action_describe(self, parameters: str):
        print_(f"{self}: starting describe with parameters: {parameters}")

        msg = self.make_request_message(MessageType.ActionDescribeEnd, {
            "inputs": ["input1", "input2"],
            "parameters": ["a", "b", parameters],
            "status": "kOfxStatusOk",
        })

        print_(f"{self}: sleeping in describe")
        sleep(4)

        print_(f"{self}: sending describe respones: {msg}")
        self.pub_socket.send(msg.to_bytes())
        print_(f"{self}: describe is finished")

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
                if msg.type_ == MessageType.ActionDescribeStart:
                    self.action_describe(msg.data["parameters"])  # XXX pass params properly
                else:
                    print_(f"{self}: Unexpected message!")

            for sock in xlist:
                print_(f"{self}: Error on socket: {sock}")

    def __str__(self) -> str:
        if self.is_proxy_plugin:
            return f"ProxyPluginInstance(plugin, id={self.instance_id})"
        else:
            return f"ProxyPluginInstance(host, id={self.instance_id})"


class MessageType(str, Enum):
    ActionDescribeStart = "ActionDescribeStart"
    ActionDescribeEnd = "ActionDescribeEnd"
    RemoteHostStarted = "RemoteHostStarted"

@dataclass
class Message:
    instance_id: int
    type_: MessageType
    data: dict

    @staticmethod
    def get_instance_prefix(instance_id) -> bytes:
        return f"{instance_id:08d}".encode("ascii")

    def to_bytes(self) -> bytes:
        return f"{self.instance_id:08d}/{self.type_}/{json.dumps(self.data)}".encode("ascii")

    @classmethod
    def from_bytes(cls, s: bytes) -> "Message":
        tmp = s.split(b"/", 2)
        return cls(
            instance_id=int(tmp[0].decode("ascii")),
            type_=MessageType(tmp[1].decode("ascii")),
            data=json.loads(tmp[2])
        )
