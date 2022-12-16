from dataclasses import dataclass
from time import sleep

import zmq
from threading import Lock, Thread

PRINT_LOCK = Lock()
def print_(*args):
    with PRINT_LOCK:
        print(*args)

context = zmq.Context()
broker_pub_socket = context.socket(zmq.PUB)
broker_sub_socket = context.socket(zmq.SUB)

def run_broker_thread():
    print_("broker: hello")
    while True:
        print_("broker: calling recv")
        data = broker_sub_socket.recv()
        print_(f"broker: got data: {data}")

def main():
    broker_pub_address = "tcp://127.0.0.1:5555"
    broker_sub_address = "tcp://127.0.0.1:5556"

    broker_pub_socket.bind(broker_pub_address)
    broker_sub_socket.bind(broker_sub_address)
    broker_sub_socket.subscribe("")  # XXX !

    instance_pub_socket = context.socket(zmq.PUB)
    instance_sub_socket = context.socket(zmq.SUB)

    instance_pub_socket.connect(broker_sub_address)
    instance_sub_socket.connect(broker_pub_address)
    instance_sub_socket.subscribe("")

    print_("starting broker_thread...")
    broker_thread = Thread(target=run_broker_thread)
    broker_thread.start()

    print_("sleeping")
    sleep(2)

    # XXX instance thread
    data = b"my message"
    print_(f"instance: calling instance_pub_socket.send: {data}")
    instance_pub_socket.send(data)
    print_("instance: after instance_pub_socket.send")


main()
