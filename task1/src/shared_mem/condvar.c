#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

// Глобальные переменные
volatile int        state;          // Текущее состояние системы
pthread_mutex_t     mutex = PTHREAD_MUTEX_INITIALIZER;  // Мьютекс для синхронизации
pthread_cond_t      cond  = PTHREAD_COND_INITIALIZER;   // Условная переменная для ожидания состояний

// Прототипы функций-обработчиков состояний
void    *state_0 (void *);
void    *state_1 (void *);
void    *state_2 (void *);
void    *state_3 (void *);

char    *progname = "condvar";  // Имя программы для вывода в логах

int main ()
{
    // Устанавливаем буферизацию вывода (построчная)
    setvbuf (stdout, NULL, _IOLBF, 0);
    
    // Инициализируем начальное состояние
    state = 0;
    
    // Объявляем идентификаторы потоков
    pthread_t t_state1, t_state0, t_state2, t_state3;
    
    // Создаем потоки для каждого состояния
    pthread_create (&t_state1, NULL, state_1, NULL);
    pthread_create (&t_state0, NULL, state_0, NULL);
    pthread_create (&t_state2, NULL, state_2, NULL);
    pthread_create (&t_state3, NULL, state_3, NULL);
    
    // Главный поток спит 20 секунд, пока работают другие потоки
    sleep (20); 
    
    // Завершаем программу
    printf ("%s:  main, exiting\n", progname);
    return 0;
}

/*
 *  Обработчик состояния 0
 *  Переходит из состояния 0 в состояние 1
 */
void *state_0 (void *arg)
{
    while (1) {
        pthread_mutex_lock (&mutex);
        
        // Ожидаем, пока состояние не станет равным 0
        while (state != 0) {
            pthread_cond_wait (&cond, &mutex);
        }
        
        // Выполняем переход и выводим информацию
        printf ("%s:  transit 0 -> 1\n", progname);
        state = 1;
        
        /* Не загружаем процессор полностью - небольшая задержка */
        usleep(100 * 1000);
        
        // Уведомляем все ожидающие потоки об изменении состояния
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock (&mutex);
    }
    return (NULL);
}

/*
 *  Обработчик состояния 1
 *  Чередуется между переходами в состояния 2 и 3
 */
void *state_1 (void *arg)
{ 
    int state_counter = 0;  // Счетчик для чередования переходов
    
    while (1) {
        pthread_mutex_lock (&mutex);
        
        // Ожидаем, пока состояние не станет равным 1
        while (state != 1) {
            pthread_cond_wait (&cond, &mutex);
        }
        
        state_counter ++;
        
        // Чередуем переходы: четные счетчики -> состояние 2, нечетные -> состояние 3
        if (state_counter % 2  == 0){
            printf ("%s:  transit 1 -> 2\n", progname);
            state = 2;
        }
        else{ 
            printf ("%s:  transit 1 -> 3\n", progname);
            state = 3;
        }
        
        // Уведомляем все ожидающие потоки
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock (&mutex);
    }
    return (NULL);
}

/*
 *  Обработчик состояния 2
 *  Переходит из состояния 2 в состояние 0
 */
void *state_2 (void *arg)
{
    while (1) {
        pthread_mutex_lock (&mutex);
        
        // Ожидаем, пока состояние не станет равным 2
        while (state != 2) {
            pthread_cond_wait (&cond, &mutex);
        }
        
        // Выполняем переход
        printf ("%s:  transit 2 -> 0\n", progname);
        state = 0;
        
        // Уведомляем все ожидающие потоки
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock (&mutex);
    }
    return (NULL);
}

/*
 *  Обработчик состояния 3
 *  Переходит из состояния 3 в состояние 0
 */
void *state_3 (void *arg)
{
    while (1) {
        pthread_mutex_lock (&mutex);
        
        // Ожидаем, пока состояние не станет равным 3
        while (state != 3) {
            pthread_cond_wait (&cond, &mutex);
        }
        
        // Выполняем переход
        printf ("%s:  transit 3 -> 0\n", progname);
        state = 0;
        
        // Уведомляем все ожидающие потоки
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock (&mutex);
    }
    return (NULL);
}