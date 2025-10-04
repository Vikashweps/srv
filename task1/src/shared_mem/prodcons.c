/*
 *  Демонстрация POSIX условных переменных на примере "Производитель и потребитель".
 *  Так как у нас всего два потока, ожидающих сигнала,    
 *  в любой момент работы одного из них мы можем просто использовать вызов
 *  pthread_cond_signal для пробуждения второго потока.
 *
*/

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

// Мьютекс и условная переменная для синхронизации
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;

// Общие разделяемые переменные
volatile int        state = 0;    // Переменная состояния: 0 - готов к производству, 1 - готов к потреблению
volatile int        product = 0;  // Продукт, производимый производителем

// Прототипы функций
void    *producer (void *);
void    *consumer (void *);
void    do_producer_work (void);
void    do_consumer_work (void);
char    *progname = "prodcons";  // Имя программы для логирования

int main ()
{
  // Устанавливаем построчную буферизацию вывода
  setvbuf (stdout, NULL, _IOLBF, 0);
  
  // Создаем потоки потребителя и производителя
  pthread_create (NULL, NULL, consumer, NULL);
  pthread_create (NULL, NULL, producer, NULL);
  
  // Главный поток ждет 20 секунд, позволяя рабочим потокам выполнить свою работу
  sleep (20);
  
  // Завершение программы
  printf ("%s:  main, exiting\n", progname);
  return 0;
}

// Функция производителя
void *producer (void *arg)
{
  while (1) {
    // Захватываем мьютекс перед доступом к общим данным
    pthread_mutex_lock (&mutex);
    
    // Ожидаем, пока состояние не станет 0 (потребитель забрал продукт)
    while (state == 1) {
      pthread_cond_wait (&cond, &mutex);
    }
    
    // Производим новый продукт
    printf ("%s:  produced %d, state %d\n", progname, ++product, state);
    state = 1;  // Устанавливаем состояние "продукт готов к потреблению"
    
    // Сигнализируем потребителю, что продукт готов
    pthread_cond_signal (&cond);
    
    // Освобождаем мьютекс
    pthread_mutex_unlock (&mutex);
    
    // Имитируем работу производителя
    do_producer_work ();
  }
  return (NULL);
}

// Функция потребителя
void *consumer (void *arg)
{
  while (1) {
    // Захватываем мьютекс перед доступом к общим данным
    pthread_mutex_lock (&mutex);
    
    // Ожидаем, пока состояние не станет 1 (продукт готов)
    while (state == 0) {
      pthread_cond_wait (&cond, &mutex);
    }
    
    // Потребляем продукт
    printf ("%s:  consumed %d, state %d\n", progname, product, state);
    state = 0;  // Устанавливаем состояние "можно производить новый продукт"
    
    // Сигнализируем производителю, что можно производить следующий продукт
    pthread_cond_signal (&cond);
    
    // Освобождаем мьютекс
    pthread_mutex_unlock (&mutex);
    
    // Имитируем работу потребителя
    do_consumer_work ();
  }
  return (NULL);
}

// Функция имитации работы производителя
void do_producer_work (void)
{
  usleep (100 * 1000);  // Задержка 100 миллисекунд
}

// Функция имитации работы потребителя
void do_consumer_work (void)
{
  usleep (100 * 1000);  // Задержка 100 миллисекунд
}