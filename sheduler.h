#pragma once
#ifndef _SHEDULER_H_
#define _SHEDULER_H_

//--------------------------------------------------------------------------------------------------
// Настройки

#define MAX_TASKS      9            // Максимальное количество задач

//--------------------------------------------------------------------------------------------------
// Типы данных

typedef void (*pFunction)(void) ;   // тип pFunction - указатель на функцию.

//--------------------------------------------------------------------------------------------------
// Функции

void os_init(void);
void os_setTask(pFunction function, uint32_t delay, uint32_t period, uint8_t priority);
void os_deleteTask(pFunction taskFunc);

#endif // _SHEDULER_H_
