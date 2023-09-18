#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <event2/event.h>
#include <event2/listener.h>

// Функция, которая будет вызываться при событии (получении данных или сигнала)
void event_callback(evutil_socket_t fd, short events, void *arg) {
    struct event *ev = (struct event *)arg;
    
    if (events & EV_READ) {
        char buffer[1024];
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            // Ошибка или соединение закрыто
            event_del(ev);
            close(fd);
        } else {
            printf("Получено %zd байт данных\n", n);
        }
    } else if (events & EV_SIGNAL) {
        // Обработка сигнала SIGHUP
        printf("Получен сигнал SIGHUP\n");
    }
}

int main() {
    // Инициализация libevent
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Ошибка инициализации libevent\n");
        return 1;
    }

    // Создание и настройка структуры для обработки сигнала SIGHUP
    struct event *sighup_event = evsignal_new(base, SIGHUP, event_callback, NULL);
    if (!sighup_event || event_add(sighup_event, NULL) < 0) {
        fprintf(stderr, "Ошибка настройки обработки сигнала SIGHUP\n");
        return 1;
    }

    // Создание и настройка сервера для принятия соединений на порту 12345
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(12345);

    struct evconnlistener *listener = evconnlistener_new_bind(
        base,
        NULL,
        NULL,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1,
        (struct sockaddr *)&sin,
        sizeof(sin)
    );

    if (!listener) {
        fprintf(stderr, "Ошибка создания сервера\n");
        return 1;
    }

    evconnlistener_set_error_cb(listener, NULL);

    // Запуск цикла обработки событий
    event_base_dispatch(base);

    // Освобождение ресурсов
    evconnlistener_free(listener);
    event_free(sighup_event);
    event_base_free(base);

    return 0;
}





