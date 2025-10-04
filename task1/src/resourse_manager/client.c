/*
 *  Клиент для менеджера ресурсов
 * 
 *   клиент подключается к серверу через UNIX сокет и отправляет команды:
 *  READ, WRITE, STATUS или произвольный текст для эхо-режима.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Путь к сокету сервера (должен совпадать с серверным)
#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"

int main(int argc, char *argv[])
{
    // Проверяем аргументы командной строки
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <команда> [данные]\n", argv[0]);
        fprintf(stderr, "Доступные команды:\n");
        fprintf(stderr, "  READ                    - Прочитать данные из устройства\n");
        fprintf(stderr, "  WRITE <текст>          - Записать текст в устройство\n");
        fprintf(stderr, "  STATUS                 - Получить статус устройства\n");
        fprintf(stderr, "  <любой текст>          - Эхо-режим (вернет полученный текст)\n");
        return EXIT_FAILURE;
    }

    // Создаем клиентский сокет
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Ошибка создания сокета");
        return EXIT_FAILURE;
    }

    // Настраиваем адрес сервера
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));          // Обнуляем структуру
    addr.sun_family = AF_UNIX;               // UNIX сокеты
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);  // Копируем путь

    // Подключаемся к серверу
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Ошибка подключения к серверу");
        close(fd);
        return EXIT_FAILURE;
    }

    // Формируем сообщение в зависимости от команды
    char message[1024];
    const char *command = argv[1];  // Первый аргумент - команда
    
    // Обработка различных команд
    if (strcmp(command, "READ") == 0) {
        // Команда чтения
        snprintf(message, sizeof(message), "READ\n");
    }
    else if (strcmp(command, "STATUS") == 0) {
        // Команда статуса
        snprintf(message, sizeof(message), "STATUS\n");
    }
    else if (strcmp(command, "WRITE") == 0) {
        // Команда записи - требует дополнительный аргумент с данными
        if (argc < 3) {
            fprintf(stderr, "Команда WRITE требует текстовые данные\n");
            fprintf(stderr, "Пример: %s WRITE \"Привет Мир\"\n", argv[0]);
            close(fd);
            return EXIT_FAILURE;
        }
        snprintf(message, sizeof(message), "WRITE %s\n", argv[2]);
    }
    else {
        // Любой другой текст - отправляем в эхо-режиме
        snprintf(message, sizeof(message), "%s\n", argv[1]);
    }

    // Отправляем сообщение серверу
    size_t len = strlen(message);
    printf("Отправка: %s", message);
    
    if (send(fd, message, len, 0) != (ssize_t)len) {
        perror("Ошибка отправки");
        close(fd);
        return EXIT_FAILURE;
    }

    // Получаем ответ от сервера
    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("Ошибка приема");
        close(fd);
        return EXIT_FAILURE;
    }
    
    buf[n] = '\0';  // Добавляем нулевой терминатор
    printf("Ответ: %s", buf);  // Выводим ответ сервера

    // Закрываем соединение
    close(fd);
    return EXIT_SUCCESS;
}