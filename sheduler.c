#include "sheduler.h"

typedef struct              // структура с описанием задачи
{
    pFunction pFunc;        // указатель на функцию
    uint32_t delay;         // задержка перед первым запуском задачи
    uint32_t period;        // период запуска задачи
    bool run;               // флаг готовности задачи к запуску
    uint8_t prioritet;      // приоритет задачи
} Task;

static volatile Task taskArray[MAX_TASKS];      // очередь задач
static volatile uint8_t arrayTail = 0;          // "хвост" очереди (количество задач)

//--------------------------------------------------------------------------------------------------
// Инициализация

inline void os_init()
{
    if (SysTick_Config(SystemCoreClock / 1000))    // время тика - 1 мс (0.001 с = 1/1000 с = 1мс)
    {
        while (1) {};       // если вернулся не ноль - ошибка  TODO
    }

    NVIC_SetPriority(SysTick_IRQn, 0); //hight priority
    NVIC_SetPriority(PendSV_IRQn, 15); //low priority
    NVIC_EnableIRQ(PendSV_IRQn);
    NVIC_EnableIRQ(SysTick_IRQn);
    __enable_irq();
}

//--------------------------------------------------------------------------------------------------
// Добавление задачи в список

void os_setTask(pFunction function, uint32_t delay, uint32_t period, uint8_t priority)
{
    if (!function)
    {
        return;
    }

    for (uint8_t i = 0; i < arrayTail; i++)         // поиск задачи в текущем списке
    {
        if (taskArray[i].pFunc == function)         // если нашли, то обновляем переменные
        {
            NVIC_DisableIRQ(SysTick_IRQn);

            taskArray[i].delay  = delay;
            taskArray[i].period = period;
            taskArray[i].prioritet = priority;
            taskArray[i].run    = false;

            NVIC_EnableIRQ(SysTick_IRQn);
            return;                                 // обновив, выходим
        }
    }
    // если такой задачи в списке нет
    if (arrayTail < MAX_TASKS)                      // и есть место,то добавляем
    {
        NVIC_DisableIRQ(SysTick_IRQn);

        taskArray[arrayTail].pFunc  = function;
        taskArray[arrayTail].delay  = delay;
        taskArray[arrayTail].period = period;
        taskArray[arrayTail].prioritet = priority;
        taskArray[arrayTail].run    = false;

        arrayTail++;                                // увеличиваем "хвост"
        NVIC_EnableIRQ(SysTick_IRQn);
    }
}

//--------------------------------------------------------------------------------------------------
// Удаление задачи из списка

void os_deleteTask(pFunction taskFunc)
{
    for (uint8_t i = 0; i < arrayTail; i++)             // проходим по списку задач
    {
        if (taskArray[i].pFunc == taskFunc)             // если задача в списке найдена
        {
            NVIC_DisableIRQ(SysTick_IRQn);

            if (i != (arrayTail - 1))                   // переносим последнюю задачу
            {
                taskArray[i] = taskArray[arrayTail - 1];// на место удаляемой
            }
            arrayTail--;                                // уменьшаем указатель "хвоста"

            NVIC_EnableIRQ(SysTick_IRQn);
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Таймерная служба

void SysTick_Handler(void)
{
    for (uint8_t i = 0; i < arrayTail; i++)         // проходим по списку задач
    {
        if (taskArray[i].delay == 0)                // если время до выполнения истекло
        {
            taskArray[i].run = true;                // взводим флаг запуска задачи
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;    // call PendSV_Handler
        }
        else
        {
            taskArray[i].delay--;               // иначе уменьшаем время
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Диспетчер РТОС, вызывается из SysTick_Handler

void PendSV_Handler()
{
    for (;;)                                                // пока не выполним всё
    {
        pFunction currentFunction = 0;
        uint8_t maxPriority = 0;
        uint8_t currentTask = 0;

        for (uint8_t i = 0; i < arrayTail; i++)             // проходим по списку задач
        {
            if (taskArray[i].run == true)                   // и из тех, что готовы к запуску
            {
                if (taskArray[i].prioritet > maxPriority)   // выбираем с большим приоритетом
                {
                    currentFunction = taskArray[i].pFunc;
                    maxPriority = taskArray[i].prioritet;
                    currentTask = i;
                }
            }
        }

        if (currentFunction)                            // если есть что выполнить
        {
            if (taskArray[currentTask].period == 0)     // если период равен 0
            {
                os_deleteTask(currentFunction);         // удаляем задачу из списка,
            }
            else
            {
                if (!taskArray[currentTask].delay)      // если задача не изменила задержку
                {
                    taskArray[currentTask].delay = taskArray[currentTask].period - 1;  // задаем ее
                }
                taskArray[currentTask].run = false;     // иначе снимаем флаг запуска
            }

            currentFunction();
        }
        else
        {
            return;  // выполнять нечего - покидаем обработчик
        }
    }
}
