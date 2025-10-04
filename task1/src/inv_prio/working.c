#include "working.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Статическая переменная для мьютекса ресурса
static pthread_mutex_t resource_mutex;

/**
 * Инициализирует мьютекс ресурса с возможностью включения наследования приоритета
 * 
 * @param enable_prio_inherit - флаг включения наследования приоритета
 * @return 0 при успехе, -1 при ошибке
 */
int init_resource_mutex(int enable_prio_inherit)
{
  pthread_mutexattr_t attr;
  
  // Инициализация атрибутов мьютекса
  if (pthread_mutexattr_init(&attr) != 0) return -1;
  
#ifdef PTHREAD_PRIO_INHERIT
  // Если система поддерживает наследование приоритета и оно запрошено
  if (enable_prio_inherit) {
    // Устанавливаем протокол наследования приоритета
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
  }
#else
  // Если система не поддерживает наследование приоритета, игнорируем параметр
  (void)enable_prio_inherit;
#endif
  
  // Инициализация мьютекса с заданными атрибутами
  int rc = pthread_mutex_init(&resource_mutex, &attr);
  
  // Освобождение атрибутов
  pthread_mutexattr_destroy(&attr);
  
  return rc == 0 ? 0 : -1;
}

/**
 * Функция занятой работы на указанное количество миллисекунд
 * 
 * @param ms - время работы в миллисекундах
 */
static void busy_ms(int ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;           // Секунды
  ts.tv_nsec = (ms % 1000) * 1000000L;  // Наносекунды
  nanosleep(&ts, NULL);            // Приостановка выполнения
}

/**
 * Имитация работы с общим ресурсом
 * 
 * @param tid - идентификатор потока для отладочной информации
 */
void working(int tid)
{
  // Выполняем 5 циклов работы с ресурсом
  for (int i = 0; i < 5; i++) {
    printf("server is working for %d - %dth start\n", tid, i);
    busy_ms(50);  // Имитация работы - 50 мс
    printf("server is working for %d - %dth end\n", tid, i);
  }
  printf("server finished all work for %d\n", tid);
}

/**
 * Низкоприоритетный поток (сервер): захватывает ресурс и держит его долгое время
 * 
 * @param arg - аргумент потока (не используется)
 * @return NULL
 */
void *server(void *arg)
{
  (void)arg;
  printf("[SERVER] стартует и захватывает ресурс\n");
  
  // Захват мьютекса - начало критической секции
  pthread_mutex_lock(&resource_mutex);
  
  // Длительная работа с ресурсом (создает условия для инверсии приоритетов)
  working(0);
  
  // Освобождение мьютекса - конец критической секции
  pthread_mutex_unlock(&resource_mutex);
  
  printf("[SERVER] освободил ресурс\n");
  return NULL;
}

/**
 * Поток со средним приоритетом: создает фоновую нагрузку на CPU
 * 
 * @param arg - аргумент потока (не используется)
 * @return NULL
 */
void *t1(void *arg)
{
  (void)arg;
  printf("[T1 mid] стартует (фоновая нагрузка)\n");
  
  // Создание фоновой нагрузки - 200 циклов по 10 мс
  for (int i = 0; i < 200; i++) {
    busy_ms(10);  // Загрузка процессора
  }
  
  printf("[T1 mid] завершился\n");
  return NULL;
}

/**
 * Поток с высоким приоритетом: пытается получить ресурс, занятый низкоприоритетным потоком
 * 
 * @param arg - аргумент потока (не используется)
 * @return NULL
 */
void *t2(void *arg)
{
  (void)arg;
  printf("[T2 high] пытается получить ресурс\n");
  
  // Попытка захвата мьютекса - может заблокироваться, если сервер его удерживает
  pthread_mutex_lock(&resource_mutex);
  
  printf("[T2 high] получил ресурс\n");
  busy_ms(20);  // Короткая работа с ресурсом
  
  pthread_mutex_unlock(&resource_mutex);
  
  printf("[T2 high] освободил ресурс и завершился\n");
  return NULL;
}