// Демонстрация проблемы, когда несколько потоков пытаются получить доступ к общей переменной
// Без использования мьютекса или других механизмов синхронизации — возникает "состояние гонки" (race condition)

#include <stdio.h>      
#include <stdlib.h>     
#include <pthread.h>    // Для работы с потоками: pthread_create, pthread_cancel и др.
#include <sched.h>      // Для управления планированием потоков (SCHED_RR, sched_param)
#include <unistd.h>     // Для sleep()

#define NumThreads      4

// volatile — указывает компилятору не оптимизировать доступ к переменной (важно в многопоточке)
volatile int var1;
volatile int var2;

//create mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *update_thread(void *);
char *progname = "mutex";

int main()
{
    pthread_t threadID[NumThreads]; // хранит ID потоков
    pthread_attr_t attrib;          // атрибуты планирования
    struct sched_param param;       // для установки приоритета
    int i, policy;                  // policy — текущая политика планирования
    setvbuf(stdout, NULL, _IOLBF, 0);//буферизация "строковая"
    var1 = var2 = 0;
    printf("%s: starting; creating threads\n", progname);

    // Настройка атрибутов потоков Round Robin, пониженный приоритет
    //pthread_getschedparam(pthread_self(), &policy, &param);
    pthread_attr_init(&attrib);
    //pthread_attr_setinheritsched(&attrib, PTHREAD_EXPLICIT_SCHED); //(не наследовать)
   // pthread_attr_setschedpolicy(&attrib, SCHED_RR);// Round Robin (циклическая)
   // param.sched_priority -= 2; // Снизить приоритет на 2 уровня
   // pthread_attr_setschedparam(&attrib, &param);

    // Создаем потоки. С каждым завершением вызова pthread_create запускается поток.
   for (i = 0; i < NumThreads; i++) {
        int result = pthread_create(&threadID[i], NULL, &update_thread, (void *)(long)i);
        if (result != 0) {
            fprintf(stderr, "pthread_create failed with error code: %d\n", result);
            exit(EXIT_FAILURE);
        }
    }
    sleep(20);
    printf("%s: stopping; cancelling threads\n", progname);
    usleep(100000);
    // Отмена потоков
    for (i = 0; i < NumThreads; i++) {
        pthread_cancel(threadID[i]);
    }
    printf("%s: all done, var1 is %d, var2 is %d\n", progname, var1, var2);
    fflush(stdout);// Принудительно сбрасываем буфер вывода
    sleep(1);
    // Уничтожаем мьютекс
    pthread_mutex_destroy(&mutex);
    exit(0);
}

void do_work()
{
    static int var3;
    var3++;
    /* Для более быстрых/медленных процессоров может потребоваться настройка этой программы путем
     * изменения частоты использования этого printf — добавления/удаления 0
     */
    if (!(var3 % 1000000)) {
        printf("%s: thread %lu did some work\n", progname, (unsigned long)pthread_self());
    }
}

void *update_thread(void *i)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while (1) {
        // ЗАХВАТЫВАЕМ МЬЮТЕКС 
        pthread_mutex_lock(&mutex);
        if (var1 != var2) {
            printf("%s: thread %ld, var1 (%d) is not equal to var2 (%d)!\n",
                   progname, (long)i, var1, var2);
            var1 = var2;
        }
      
        var1++;
        var2++;
        pthread_mutex_unlock(&mutex);
        do_work();
        pthread_testcancel();
    }
    return NULL;  
}