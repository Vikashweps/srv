#include <stdlib.h>  
#include <stdio.h>   
#include <pthread.h> // Для работы с потоками (pthread_create, pthread_join и т.д.)
#include <sched.h>   // Для управления планированием и приоритетами потоков 
#include <unistd.h>  // Для usleep (микрозадержки), pause и др.

// Объявление функций потоков
void *sense(void* arg);           // считывает ввод от пользователя
void *stateOutput(void* arg);     // Поток, выводящий сообщение при изменении состояния
void *userInterface(void* arg);   // Поток, отображающий "интерфейс" в зависимости от состояния

// Вспомогательная функция: проверяет, является ли символ допустимым состоянием
short isRealState(char s);

char state;              // Текущее состояние системы ('N', 'R', 'D' и их строчные варианты)
short changed;           // Флаг: было ли изменение состояния? (TRUE/FALSE)

pthread_cond_t stateCond;   // Условная переменная — для уведомления потока stateOutput об изменении
pthread_mutex_t stateMutex; // Мьютекс — для безопасного доступа к state и changed

#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {

    printf("Hello World!\n");  

    state = 'N';  // Начальное состояние — "Не готов"
    changed = FALSE;

    pthread_cond_init(&stateCond, NULL);   // Инициализируем условную переменную
    pthread_mutex_init(&stateMutex, NULL); // Инициализируем мьютекс

    // Объявляем идентификаторы потоков:
    pthread_t sensorThread;       // Поток ввода (sense)
    pthread_t stateOutputThread;  // Поток вывода изменений (stateOutput)
    pthread_t userThread;         // Поток отображения интерфейса (userInterface)

    // Создаем потоки:
    pthread_create(&sensorThread, NULL, sense, NULL);
    pthread_create(&stateOutputThread, NULL, stateOutput, NULL);
    pthread_create(&userThread, NULL, userInterface, NULL);

    // Ожидаем завершения всех потоков 
    pthread_join(sensorThread, NULL);
    pthread_join(stateOutputThread, NULL);
    pthread_join(userThread, NULL);

    printf("Успешный выход!\n");

    // Освобождаем ресурсы синхронизации:
    pthread_mutex_destroy(&stateMutex);
    pthread_cond_destroy(&stateCond);

    return EXIT_SUCCESS;  // Возвращаем код успешного завершения
}

// ПОТОК: sense — считывает ввод от пользователя
void *sense(void* arg) {
    char prevState = ' ';  // Предыдущее состояние (инициализируем пробелом, чтобы первое изменение сработало)

    while (TRUE) {  // Бесконечный цикл — поток работает всегда
        char tempState;  // Временная переменная для ввода
        scanf("%c", &tempState);
        usleep(10000);
        pthread_mutex_lock(&stateMutex); // Захватываем мьютекс — блокируем доступ к state и changed для других потоков
       
        if (isRealState(tempState)) { // Проверяем, является ли введённый символ допустимым состоянием
            state = tempState;  // Обновляем текущее состояние
        }

        // Проверяем, изменилось ли состояние по сравнению с предыдущим
        // prevState != (state ^ ' ') — хитрая проверка на регистр: 'N' и 'n' считаются одинаковыми
        // (работает, потому что 'N' ^ ' ' = 'n', и наоборот — это особенность ASCII)
        if (prevState != state && prevState != (state ^ ' ')) {
            changed = TRUE;                  // Устанавливаем флаг изменения
            pthread_cond_signal(&stateCond); // Посылаем сигнал потоку stateOutput
        }
        prevState = state;// Обновляем предыдущее состояние

        pthread_mutex_unlock(&stateMutex); // Освобождаем мьютекс
    }
    return NULL;  
}

// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ: проверка допустимости состояния
short isRealState(char s) {
    short real = FALSE;  // По умолчанию — недопустимое состояние

    // Проверяем, входит ли символ в список допустимых:
    if (s == 'R' || s == 'r')  // Готов
        real = TRUE;
    else if (s == 'N' || s == 'n')  // Не готов
        real = TRUE;
    else if (s == 'D' || s == 'd')  // Режим работы
        real = TRUE;

    return real;
}

// ПОТОК: stateOutput — выводит сообщение при изменении состояния
void *stateOutput(void* arg) {
    changed = FALSE;  // Изначально изменений нет

    while (TRUE) {
        pthread_mutex_lock(&stateMutex);

        // Ждём, пока changed не станет TRUE (поток "засыпает", освобождая CPU)
        while (!changed) {
            pthread_cond_wait(&stateCond, &stateMutex);  // Освобождает мьютекс и ждёт сигнала
            // После пробуждения мьютекс автоматически захватывается снова
        }

        // Выводим сообщение о новом состоянии:
        printf("Состояние изменилось! Теперь оно в ");
        if (state == 'n' || state == 'N')
            printf("состоянии Не готов\n");
        else if (state == 'r' || state == 'R')
            printf("состоянии Готов\n");
        else if (state == 'd' || state == 'D')
            printf("Режиме работы\n");

        // Сбрасываем флаг изменения
        changed = FALSE;
        pthread_mutex_unlock(&stateMutex);
    }
    return NULL;
}

// ПОТОК: userInterface — отображает "визуальный интерфейс"
void *userInterface(void* arg) {
    while (TRUE) {
        if (state == 'n' || state == 'N')
            printf("___________________________________________________\n");
        else if (state == 'r' || state == 'R')
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        else if (state == 'd' || state == 'D')
            printf("\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/^\\_/\n");

        // Задержка 1 секунда (1000000 микросекунд)
        usleep(1000000);
    }
    return NULL;
}