import threading
import time

event = threading.Event()

def producer():
    while True:
        print("Поток-поставщик:инициализированно условное событие")
        event.set()
        time.sleep(1)

def consumer():
    while True:
        print("Поток-потребитель: ожидание условного события")
        event.wait()
        event.clear()
        print("Поток-потребитель: условное событие получено")

producer_thread = threading.Thread(target=producer)
consumer_thread = threading.Thread(target=consumer)

producer_thread.start()
consumer_thread.start()

try:
    producer_thread.join()
    consumer_thread.join()
except KeyboardInterrupt:
    print("Программа завершена")
