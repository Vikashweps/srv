#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <command> [data]\n", argv[0]);
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  READ                    - Read from device\n");
    fprintf(stderr, "  WRITE <text>           - Write text to device\n");
    fprintf(stderr, "  STATUS                 - Get device status\n");
    fprintf(stderr, "  <any text>             - Echo mode\n");
    return EXIT_FAILURE;
  }

  // Создаем сокет
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  // Настраиваем адрес сервера
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

  // Подключаемся к серверу
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("connect");
    close(fd);
    return EXIT_FAILURE;
  }

  // Формируем сообщение в зависимости от команды
  char message[1024];
  const char *command = argv[1];
  
  if (strcmp(command, "READ") == 0) {
    snprintf(message, sizeof(message), "READ\n");
  }
  else if (strcmp(command, "STATUS") == 0) {
    snprintf(message, sizeof(message), "STATUS\n");
  }
  else if (strcmp(command, "WRITE") == 0) {
    if (argc < 3) {
      fprintf(stderr, "WRITE command requires text data\n");
      fprintf(stderr, "Example: %s WRITE \"Hello World\"\n", argv[0]);
      close(fd);
      return EXIT_FAILURE;
    }
    snprintf(message, sizeof(message), "WRITE %s\n", argv[2]);
  }
  else {
    // Любой другой текст - эхо-режим
    snprintf(message, sizeof(message), "%s\n", argv[1]);
  }

  // Отправляем сообщение серверу
  size_t len = strlen(message);
  printf("Sending: %s", message);
  
  if (send(fd, message, len, 0) != (ssize_t)len) {
    perror("send");
    close(fd);
    return EXIT_FAILURE;
  }

  // Получаем ответ от сервера
  char buf[1024];
  ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
  if (n < 0) {
    perror("recv");
    close(fd);
    return EXIT_FAILURE;
  }
  buf[n] = '\0';
  printf("response: %s", buf);

  close(fd);
  return EXIT_SUCCESS;
}