#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Глобальные переменные для счетчика и флагов сигналов
static volatile unsigned counter = 0;                    // Счетчик событий
static volatile sig_atomic_t got_sigint = 0;            // Флаг получения SIGINT
static volatile sig_atomic_t got_sigterm = 0;           // Флаг получения SIGTERM
static volatile sig_atomic_t got_sigusr1 = 0;           // Флаг получения SIGUSR1
static volatile sig_atomic_t got_sigusr2 = 0;           // Флаг получения SIGUSR2
static const char *progname = "int";                    // Имя программы
static struct termios orig_termios;                     // Оригинальные настройки терминала

// Восстанавливает оригинальные настройки терминала при выходе
static void restore_terminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Обработчик сигнала ALARM - подсчитывает события
static void on_alarm(int signo) {
  (void)signo;
  if (++counter == 100) {
    counter = 0;
    write(STDOUT_FILENO, "100 events\n", 11);  // Выводим каждые 100 событий
  }
}

// Обработчики сигналов - устанавливают соответствующие флаги
static void handle_sigint(int signo) { (void)signo; got_sigint = 1; }
static void handle_sigterm(int signo) { (void)signo; got_sigterm = 1; }
static void handle_sigusr1(int signo) { (void)signo; got_sigusr1 = 1; }
static void handle_sigusr2(int signo) { (void)signo; got_sigusr2 = 1; }

// Переключает терминал в raw-режим (неканонический режим без эха)
static int enable_raw_mode(void) {
  // Получаем текущие настройки терминала
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return -1;
  
  struct termios raw = orig_termios;
  // Отключаем канонический режим и эхо
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;   // Неблокирующее чтение - возвращаться сразу, даже если нет данных
  raw.c_cc[VTIME] = 0;  // Без таймаута
  
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) return -1;
  
  // Регистрируем функцию восстановления терминала при выходе
  atexit(restore_terminal);
  return 0;
}

int main(void) {
  // Настройка буферизации вывода (построчная)
  setvbuf(stdout, NULL, _IOLBF, 0);
  
  printf("%s: starting...\n", progname);
  printf("Поддерживаемые сигналы: SIGINT(Ctrl+C), SIGTERM, SIGUSR1, SIGUSR2.\n");
  printf("Периодические сигналы: SIGALRM каждые 10ms\n");
  printf("Замечание: SIGKILL нельзя перехватить или обработать на Linux.\n");
  printf("Нажмите 'q' для выхода.\n");

  // Включаем raw-режим терминала
  if (enable_raw_mode() == -1) {
    perror("termios");
    return EXIT_FAILURE;
  }

  // Настройка обработчиков сигналов
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_alarm;
  sigemptyset(&sa.sa_mask);
  
  // Устанавливаем обработчик для SIGALRM
  if (sigaction(SIGALRM, &sa, NULL) == -1) {
    perror("sigaction SIGALRM");
    return EXIT_FAILURE;
  }

  // Настройка обработчиков пользовательских сигналов
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, NULL);
  
  sa.sa_handler = handle_sigterm;
  sigaction(SIGTERM, &sa, NULL);
  
  sa.sa_handler = handle_sigusr1;
  sigaction(SIGUSR1, &sa, NULL);
  
  sa.sa_handler = handle_sigusr2;
  sigaction(SIGUSR2, &sa, NULL);

  // Настройка таймера для периодической отправки SIGALRM
  struct itimerval itv;
  // Период 10 мс
  itv.it_interval.tv_sec = 0;
  itv.it_interval.tv_usec = 10000; // 10ms
  itv.it_value = itv.it_interval;  // Стартовое значение равно интервалу
  
  if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
    perror("setitimer");
    return EXIT_FAILURE;
  }
  
  printf("Таймер запущен: SIGALRM каждые 10ms\n");
  printf("Ожидание событий...\n\n");

  // Основной цикл программы: опрашиваем stdin и проверяем флаги сигналов
  for (;;) {
    // Проверка и обработка полученных сигналов
    if (got_sigint)  { got_sigint = 0;  printf("%s: получен SIGINT (Ctrl+C)\n", progname); }
    if (got_sigterm) { got_sigterm = 0; printf("%s: получен SIGTERM\n", progname); }
    if (got_sigusr1) { got_sigusr1 = 0; printf("%s: получен SIGUSR1\n", progname); }
    if (got_sigusr2) { got_sigusr2 = 0; printf("%s: получен SIGUSR2\n", progname); }
  
    // Чтение ввода с клавиатуры (неблокирующее)
    char ch;
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    
    if (n == 1) {
      // Обработка нажатых клавиш
      if (ch == 'q' || ch == 'Q') {
        printf("%s: выход по клавише 'q'\n", progname);
        break;
      }
      else if (ch == 'r' || ch == 'R') {
        // Сброс счетчика таймера
        counter = 0;
        printf("%s: счетчик таймера сброшен\n", progname);
      }
      else if (ch == 'i' || ch == 'I') {
        // Показать информацию о текущем состоянии
        printf("%s: текущее значение счетчика: %u\n", progname, counter);
      }
      else if (ch == '\n' || ch == '\r') {
        // Игнорировать переводы строк
      }
      else {
        printf("%s: нажата клавиша '%c' (0x%02x)\n", progname, ch, (unsigned char)ch);
        printf("Доступные команды: 'q'-выход, 'r'-сброс счетчика, 'i'-информация\n");
      }
    } 
    else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      // Ошибка чтения (кроме "данные недоступны")
      perror("read");
      break;
    }

    // Короткая пауза для уменьшения нагрузки на CPU
    usleep(10 * 1000); // 10ms
  }

  // Останавливаем таймер перед выходом
  memset(&itv, 0, sizeof(itv));
  setitimer(ITIMER_REAL, &itv, NULL);
  printf("%s: таймер остановлен\n", progname);
  
  return EXIT_SUCCESS;
}