/*Этот модуль демонстрирует использование семафоров POSIX.
 *
 *  Принцип работы:
 *      Создаётся счётный семафор, инициализированный значением 0.
 *      Запускаются 5 потоков-"потребителей", каждый из которых пытается захватить семафор (блокируется).
 *      Запускается 1 поток-"производитель", который периодически увеличивает значение семафора (post),
 *      тем самым разблокируя одного из ожидающих потребителей.
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

sem_t   *mySemaphore;// Указатель на семафор
void    *producer (void *);
void    *consumer (void *);
char    *progname = "semex";

#define SEM_NAME "/Semex"
// Для совместимости с Linux/macOS используем именованные семафоры по умолчанию
#define Named 1

int main ()
{
    int i;
    setvbuf (stdout, NULL, _IOLBF, 0);
#ifdef  Named
    mySemaphore = sem_open (SEM_NAME, O_CREAT, S_IRWXU, 0);
   /* не используется совместно с другими процессами, поэтому немедленно удаляется */
    sem_unlink( SEM_NAME );
#else   // Named
    mySemaphore = malloc (sizeof (sem_t));
    sem_init (mySemaphore, 0, 0);
#endif  // Named
// Массивы для хранения ID потоков
    pthread_t consumers[5];
    pthread_t producerThread;
    for (i = 0; i < 5; i++) {
        pthread_create (&consumers[i], NULL, consumer, (void *)(long)i);
    }
    pthread_create (&producerThread, NULL, producer, (void *) 1);
    sleep (20);     // let the threads run
    printf ("%s:  main, exiting\n", progname);
    return 0;
}

void *producer (void *i)
{
    while (1) {
        sleep (1);// Увеличиваем значение семафора на 1 → разблокирует одного ожидающего потребителя
        printf ("%s:  (producer %ld), posted semaphore\n", progname, (long)i);
        sem_post (mySemaphore);
    }
    return (NULL);
}

void *consumer (void *i)
{
    while (1) {
        sem_wait (mySemaphore);
        printf ("%s:  (consumer %ld) got semaphore\n", progname, (long)i);
    }
    return (NULL);
}