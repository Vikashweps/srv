#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>

/**
 * Устанавливает приоритет и политику планирования для потока
 * 
 * @param attr - атрибуты потока для настройки
 * @param policy - политика планирования (SCHED_FIFO, SCHED_RR и т.д.)
 * @param prio - значение приоритета
 * @return 0 при успехе, код ошибки при неудаче
 */
static int set_thread_priority(pthread_attr_t *attr, int policy, int prio)
{
  // Инициализация атрибутов потока
  pthread_attr_init(attr);
  // Устанавливаем явное наследование параметров планирования
  pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  // Устанавливаем политику планирования
  pthread_attr_setschedpolicy(attr, policy);
  
  // Настраиваем параметры планирования
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = prio;  // Устанавливаем приоритет
  
  return pthread_attr_setschedparam(attr, &sp);
}

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  // Настраиваем политику SCHED_FIFO и приоритеты потоков:
  const int policy = SCHED_FIFO;  // Политика планирования FIFO (первым пришел - первым обслужен)
  const int prio_server = 10;     // Низкий приоритет для сервера
  const int prio_t1 = 20;         // Средний приоритет для потока t1
  const int prio_t2 = 30;         // Высокий приоритет для потока t2

  // Инициализация мьютекса С наследованием приоритета - для решения проблемы инверсии
  // Параметр 1 включает PTHREAD_PRIO_INHERIT
  if (init_resource_mutex(1) != 0) {
    perror("init_resource_mutex");
    return EXIT_FAILURE;
  }

  // Объявление атрибутов для каждого потока
  pthread_attr_t attr_server, attr_t1, attr_t2;
  
  // Настройка атрибутов для потока сервера (низкий приоритет)
  if (set_thread_priority(&attr_server, policy, prio_server) != 0) {
    perror("attr_server");
  }
  
  // Настройка атрибутов для потока t1 (средний приоритет)
  if (set_thread_priority(&attr_t1, policy, prio_t1) != 0) {
    perror("attr_t1");
  }
  
  // Настройка атрибутов для потока t2 (высокий приоритет)
  if (set_thread_priority(&attr_t2, policy, prio_t2) != 0) {
    perror("attr_t2");
  }

  // Объявление идентификаторов потоков
  pthread_t th_server, th_t1, th_t2;

  // Запуск потока сервера (низкий приоритет) - он захватит ресурс и будет долго его удерживать
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }
  
  // Небольшая задержка, чтобы сервер успел захватить мьютекс первым
  usleep(50 * 1000);  // 50 миллисекунд

  // Запуск потока с высоким приоритетом (t2) - он попытается взять мьютекс
  // Благодаря наследованию приоритета, сервер временно получит приоритет t2
  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }
  
  // Запуск потока со средним приоритетом (t1) - создает фоновую нагрузку
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  // Ожидание завершения всех потоков
  void *st;  // Указатель для хранения статуса завершения
  pthread_join(th_t1, &st);     // Ожидание завершения t1
  pthread_join(th_t2, &st);     // Ожидание завершения t2
  pthread_join(th_server, &st); // Ожидание завершения server

  // Итоговое сообщение - в этом сценарии задержка t2 должна быть минимальной
  printf("scenario_1: завершено. С наследованием приоритета t2 не должен блокироваться надолго.\n");
  return EXIT_SUCCESS;
}