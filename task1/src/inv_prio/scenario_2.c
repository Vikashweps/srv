#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static int set_thread_priority(pthread_attr_t *attr, int policy, int prio)
{
  pthread_attr_init(attr);
  pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(attr, policy);
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = prio;
  return pthread_attr_setschedparam(attr, &sp);
}

// Функция для измерения времени
long long current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  const int policy = SCHED_FIFO;
  const int prio_server = 10;
  const int prio_t1 = 20;
  const int prio_t2 = 30;

  printf(" Тест 1: Мьютекс БЕЗ наследования приоритета \n");
  
  // Мьютекс без наследования приоритета — демонстрация инверсии
  if (init_resource_mutex(0) != 0) {  // 0 = без защиты
    perror("init_resource_mutex");
    return EXIT_FAILURE;
  }

  pthread_attr_t attr_server, attr_t1, attr_t2;
  set_thread_priority(&attr_server, policy, prio_server);
  set_thread_priority(&attr_t1, policy, prio_t1);
  set_thread_priority(&attr_t2, policy, prio_t2);

  pthread_t th_server, th_t1, th_t2;

  long long start_time = current_time_us();

  // Запускаем сервер (низкий приоритет)
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }

  usleep(50 * 1000); // Даем серверу захватить мьютекс

  // Запускаем высокий приоритет (ждет мьютекс) и средний (фон)
  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  // Ждём завершения
  void *st;
  pthread_join(th_t1, &st);
  pthread_join(th_t2, &st);
  pthread_join(th_server, &st);

  long long end_time = current_time_us();
  long long duration_no_protection = end_time - start_time;
  
  printf("Время выполнения БЕЗ защиты: %lld мкс\n\n", duration_no_protection);

  // Пауза между тестами
  usleep(1000000);

  printf(" Тест 2: Мьютекс С наследованием приоритета (PRIO_INHERIT) \n");
  
  // Мьютекс с наследованием приоритета
  if (init_resource_mutex(1) != 0) {  // 1 = PRIO_INHERIT
    perror("init_resource_mutex PRIO_INHERIT");
    return EXIT_FAILURE;
  }

  start_time = current_time_us();

  // Запускаем потоки снова
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }

  usleep(50 * 1000);

  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  pthread_join(th_t1, &st);
  pthread_join(th_t2, &st);
  pthread_join(th_server, &st);

  end_time = current_time_us();
  long long duration_with_inherit = end_time - start_time;
  
  printf("Время выполнения С наследованием: %lld мкс\n\n", duration_with_inherit);

  // Пауза между тестами
  usleep(1000000);

  printf(" Тест 3: Мьютекс С ceiling приоритетом (PRIO_PROTECT) \n");
  
  // Мьютекс с ceiling приоритетом
  if (init_resource_mutex(2) != 0) {  // 2 = PRIO_PROTECT
    perror("init_resource_mutex PRIO_PROTECT");
    return EXIT_FAILURE;
  }

  start_time = current_time_us();

  // Запускаем потоки снова
  if (pthread_create(&th_server, &attr_server, server, NULL) != 0) {
    perror("pthread_create server");
    return EXIT_FAILURE;
  }

  usleep(50 * 1000);

  if (pthread_create(&th_t2, &attr_t2, t2, NULL) != 0) {
    perror("pthread_create t2");
    return EXIT_FAILURE;
  }
  if (pthread_create(&th_t1, &attr_t1, t1, NULL) != 0) {
    perror("pthread_create t1");
    return EXIT_FAILURE;
  }

  pthread_join(th_t1, &st);
  pthread_join(th_t2, &st);
  pthread_join(th_server, &st);

  end_time = current_time_us();
  long long duration_with_protect = end_time - start_time;
  
  printf("Время выполнения С ceiling: %lld мкс\n\n", duration_with_protect);

  // Сравнение результатов

  printf("БЕЗ защиты:          %lld мкс\n", duration_no_protection);
  printf("С наследованием:     %lld мкс\n", duration_with_inherit);
  printf("С ceiling:           %lld мкс\n", duration_with_protect);
  
  long long improvement_inherit = duration_no_protection - duration_with_inherit;
  long long improvement_protect = duration_no_protection - duration_with_protect;
  
  printf("\nУлучшение с наследованием: %lld мкс (%.1f%%)\n", 
         improvement_inherit, (improvement_inherit * 100.0) / duration_no_protection);
  printf("Улучшение с ceiling:      %lld мкс (%.1f%%)\n", 
         improvement_protect, (improvement_protect * 100.0) / duration_no_protection);

  // Освобождение ресурсов
  pthread_attr_destroy(&attr_server);
  pthread_attr_destroy(&attr_t1);
  pthread_attr_destroy(&attr_t2);

  return EXIT_SUCCESS;
}