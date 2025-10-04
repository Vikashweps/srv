/*
 *  Менеджер ресурсов
 * 
 *   сервер реализует менеджер ресурсов с использованием UNIX сокетов.
 *  Он обрабатывает команды READ, WRITE, STATUS от клиентов и сохраняет данные
 *  в буфере устройства. Каждое подключение обрабатывается в отдельном потоке.
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Константы для настройки сервера
#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"  // Путь к UNIX сокету
#define BUFFER_SIZE 1024                              // Размер буфера устройства

// Глобальные переменные сервера
static const char *progname = "resmgr";               // Имя программы для логов
static int optv = 1;                                  // Флаг verbose режима (всегда включен для отладки)
static int listen_fd = -1;                            // Дескриптор слушающего сокета
static char device_buffer[BUFFER_SIZE];               // Буфер для хранения данных устройства
static size_t device_size = 0;                        // Текущий размер данных в буфере
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;  // Мьютекс для защиты буфера

// Прототипы функций
static void install_signals(void);                    // Установка обработчиков сигналов
static void on_signal(int signo);                     // Обработчик сигналов завершения
static void *client_thread(void *arg);                // Функция потока для обработки клиента

int main(int argc, char *argv[])
{
    // Настройка буферизации вывода (построчная)
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf(" %s: START PROGRAMM \n", progname);
    printf("PID процесса: %d\n", getpid());
    install_signals();  // Устанавливаем обработчики сигналов

    // Создаём UNIX сокет для межпроцессного взаимодействия
    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("Ошибка создания сокета");
        return EXIT_FAILURE;
    }
    printf("Сокет создан: fd=%d\n", listen_fd);

    // Настраиваем адрес сокета
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));          // Обнуляем структуру адреса
    addr.sun_family = AF_UNIX;               // Указываем семейство UNIX сокетов
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);  // Копируем путь

    // Удаляем старый сокетный файл, если он существует
    unlink(EXAMPLE_SOCK_PATH);

    // Привязываем сокет к адресу
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Ошибка привязки сокета");
        close(listen_fd);
        return EXIT_FAILURE;
    }
    printf("Сокет привязан к: %s\n", EXAMPLE_SOCK_PATH);

    // Переводим сокет в режим прослушивания
    if (listen(listen_fd, 8) == -1) {
        perror("Ошибка перевода в режим прослушивания");
        close(listen_fd);
        unlink(EXAMPLE_SOCK_PATH);
        return EXIT_FAILURE;
    }
    printf("Ожидание подключений...\n");

    // Информация для пользователя
    printf("\n SERVER WORKING\n");
    printf("Для подключения используйте команды:\n");
    printf("  ./client WRITE \"Hello World\"  - записать текст в устройство\n");
    printf("  ./client READ                   - прочитать данные из устройства\n");
    printf("  ./client STATUS                 - получить статус устройства\n");
   
    // Основной цикл сервера: принятие подключений
    while (1) {
        printf("Ожидание нового подключения...\n");
        
        // Принимаем входящее подключение
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno == EINTR) continue;  // Если прервано сигналом - продолжаем
            perror("Ошибка accept");
            break;
        }

        printf(">>> НОВОЕ ПОДКЛЮЧЕНИЕ: fd=%d\n", client_fd);

        // Создаем отдельный поток для обработки клиента
        pthread_t th;
        if (pthread_create(&th, NULL, client_thread, (void *)(long)client_fd) != 0) {
            perror("Ошибка создания потока");
            close(client_fd);
            continue;
        }
        
        // Отсоединяем поток - он завершится самостоятельно
        pthread_detach(th);
    }

    // Завершаем работу сервера
    if (listen_fd != -1) close(listen_fd);
    unlink(EXAMPLE_SOCK_PATH);
    return EXIT_SUCCESS;
}

/**
 * Функция потока для обработки клиентского подключения
 * 
 * @param arg - дескриптор клиентского сокета (передан как void*)
 * @return NULL
 */
static void *client_thread(void *arg)
{
    int fd = (int)(long)arg;  // Дескриптор клиентского сокета
    char buf[1024];           // Буфер для приема данных
    
    printf("Поток запущен для обработки fd=%d\n", fd);

    // Основной цикл обработки команд от клиента
    for (;;) {
        // Принимаем данные от клиента
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n == 0) {
            // Клиент закрыл соединение
            printf("<<< КЛИЕНТ ОТКЛЮЧИЛСЯ: fd=%d\n", fd);
            break;
        }
        if (n < 0) {
            // Ошибка приема (кроме прерывания сигналом)
            if (errno == EINTR) continue;
            perror("Ошибка recv");
            break;
        }
        
        // Добавляем нулевой терминатор для работы со строкой
        buf[n] = '\0';
        printf("ПОЛУЧЕНО от fd=%d: %s", fd, buf);

        // Убираем символ новой строки для удобства обработки
        if (n > 0 && buf[n-1] == '\n') {
            buf[n-1] = '\0';
        }

        // Захватываем мьютекс для безопасной работы с буфером
        pthread_mutex_lock(&buffer_mutex);
        
        // Обработка команды READ - чтение данных из буфера
        if (strncmp(buf, "READ", 4) == 0) {
            printf("Обработка команды READ\n");
            char response[128];
            snprintf(response, sizeof(response), "OK [%.*s]\n", 
                    (int)device_size, device_buffer);
            send(fd, response, strlen(response), 0);
            printf("Отправлено: %s", response);
        }
        // Обработка команды WRITE - запись данных в буфер
        else if (strncmp(buf, "WRITE", 5) == 0) {
            printf("Обработка команды WRITE\n");
            const char *data = buf + 6;  // Пропускаем "WRITE " чтобы получить данные
            size_t len = strlen(data);
            
            // Проверяем, помещаются ли данные в буфер
            if (device_size + len < BUFFER_SIZE) {
                // Копируем данные в буфер
                memcpy(device_buffer + device_size, data, len);
                device_size += len;
                send(fd, "OK Written\n", 11, 0);
                printf("Буфер обновлен: размер=%zu, содержимое=[%.*s]\n", 
                       device_size, (int)device_size, device_buffer);
            } else {
                // Буфер переполнен
                send(fd, "ERR Buffer full\n", 16, 0);
                printf("БУФЕР ПЕРЕПОЛНЕН: размер=%zu\n", device_size);
            }
        }
        // Обработка команды STATUS - получение статуса буфера
        else if (strncmp(buf, "STATUS", 6) == 0) {
            printf("Обработка команды STATUS\n");
            char response[64];
            snprintf(response, sizeof(response), "OK size=%zu\n", device_size);
            send(fd, response, strlen(response), 0);
            printf("Отправлено: %s", response);
        }
        // Режим эхо - для любых других команд
        else {
            printf("Обработка команды ECHO\n");
            char response[1024];
            snprintf(response, sizeof(response), "ECHO: %s\n", buf);
            send(fd, response, strlen(response), 0);
            printf("Отправлено: %s", response);
        }
        
        // Освобождаем мьютекс
        pthread_mutex_unlock(&buffer_mutex);
    }

    // Закрываем клиентский сокет и завершаем поток
    close(fd);
    printf("Поток завершен для fd=%d\n", fd);
    return NULL;
}

/**
 * Устанавливает обработчики сигналов для корректного завершения сервера
 */
static void install_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;              // Указываем функцию-обработчик
    sigemptyset(&sa.sa_mask);               // Инициализируем маску сигналов
    sigaction(SIGINT, &sa, NULL);           // Ctrl+C
    sigaction(SIGTERM, &sa, NULL);          // Сигнал завершения
}

/**
 * Обработчик сигналов для корректного завершения работы сервера
 * 
 * @param signo - номер полученного сигнала
 */
static void on_signal(int signo)
{
    printf("\n %s: END WORKING %d \n", progname, signo);
    if (listen_fd != -1) close(listen_fd);  // Закрываем слушающий сокет
    unlink(EXAMPLE_SOCK_PATH);              // Удаляем файл сокета
    _exit(0);                               // Немедленное завершение
}