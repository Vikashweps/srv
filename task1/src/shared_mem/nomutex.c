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

// Объявление функции потока
void *update_thread(void *);

// Имя программы для вывода в лог
char *progname = "nomutex";

int main()
{
    pthread_t threadID[NumThreads];
    pthread_attr_t attrib;// Атрибуты потока — для планирования
    struct sched_param param; // Параметры планирования — в частности, приоритет
    int i, policy;  // policy — текущая политика планирования
    setvbuf(stdout, NULL, _IOLBF, 0);// Устанавливаем буферизацию stdout на "строковую" — чтобы каждая строка сразу выводилась
    var1 = var2 = 0;
    printf("%s: starting; creating threads\n", progname);

    /*
     * Мы хотим создать потоки с политикой Round Robin (SCHED_RR)
     * и пониженным приоритетом, чтобы они не "забивали" систему полностью.
     * Это особенно важно, так как потоки будут активно нагружать CPU.
     */

    // Получаем текущую политику и параметры планирования главного потока
    pthread_getschedparam(pthread_self(), &policy, &param);

    // Инициализируем атрибуты потока
    pthread_attr_init(&attrib);

    // Указываем, что потоки должны использовать явно заданные атрибуты планирования (не наследовать от родителя)
    pthread_attr_setinheritsched(&attrib, PTHREAD_EXPLICIT_SCHED);

    // Устанавливаем политику планирования — Round Robin (циклическая)
    pthread_attr_setschedpolicy(&attrib, SCHED_RR);

    // Уменьшаем приоритет
    param.sched_priority -= 2;
    pthread_attr_setschedparam(&attrib, &param);

    for (i = 0; i < NumThreads; i++) {
        // Передаём в поток его номер (i) в виде указателя (кастуем long → void*)
        pthread_create(&threadID[i], &attrib, &update_thread, (void *)(long)i);
    }

    sleep(20);
    printf("%s: stopping; cancelling threads\n", progname);

    for (i = 0; i < NumThreads; i++) {
        pthread_cancel(threadID[i]);
    }
    printf("%s: all done, var1 is %d, var2 is %d\n", progname, var1, var2);
    fflush(stdout);// Принудительно сбрасываем буфер вывода
    sleep(1);
    exit(0);
}
void do_work()
{
    static int var3;
    var3++;
    if (!(var3 % 10000000)) {
        printf("%s: thread %lu did some work\n", progname, (unsigned long)pthread_self());
    }
}

void *update_thread(void *i)
{
    // На Linux устанавливаем асинхронную отмену — поток может быть прерван в любой момент
    // (обычно используется с осторожностью, но здесь — для простоты демонстрации)
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
        // Проверяем: равны ли var1 и var2?
        // Логически, если потоки работают корректно — они всегда должны быть равны.
        if (var1 != var2) {
            printf("%s: thread %ld, var1 (%d) is not equal to var2 (%d)!\n",
                   progname, (long)i, var1, var2);
            var1 = var2;
        }
        do_work();
        var1++;

        // ⚠️ ВАЖНО: между этими двумя операциями — "дыра", где другой поток может вклиниться!
        // var1++; ... (здесь другой поток может изменить var2) ... var2++;
        var2++;

        // Раскомментируйте, если на мощном процессоре проблема не проявляется:
        // sched_yield(); // Явно уступаем процессор другим потокам — усиливает race condition
    }
    return NULL;  
}