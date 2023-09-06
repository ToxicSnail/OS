import threading
import time

event = threading.Event()

def producer():
    while True:
        print("Potok-postavshik: iniitsiirovano uslovnoe sobitie")
        event.set()
        time.sleep(1)

def consumer():
    while True:
        print("Potok-potrebitel: oghidanie uslovnogo sobitiya")
        event.wait()
        event.clear()
        print("Potok-potrebitel: uslovnoe sobitie polucheno")

producer_thread = threading.Thread(target=producer)
consumer_thread = threading.Thread(target=consumer)

producer_thread.start()
consumer_thread.start()

try:
    producer_thread.join()
    consumer_thread.join()
except KeyboardInterrupt:
    print("Prodramma zavershena")