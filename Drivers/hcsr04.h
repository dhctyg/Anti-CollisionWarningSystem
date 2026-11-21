#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f10x.h"

typedef enum
{
    HCSR04_IDLE,
    HCSR04_WAITING_ECHO,
    HCSR04_GAPTIME
} Hcsr04_WorkStateTypeDef;

typedef enum
{
    HCSR04_WAITING_RISING,
    HCSR04_WAITING_FALLING,
    HCSR04_RESULT_DONE,
    HCSR04_RESULT_WRONG
} Hcsr04_ResultStateTypeDef;

#define HCSR04_GAP_MS           70U
#define HCSR04_TIMEOUT_MS       30U

/* 测距操作由 TaskHandle 统一执行，C文件中可以将测距、测距处理等函数独立 */
void Hcsr04_TaskHandle(void);
void Hcsr04_IrqHandle(void);
uint32_t Hcsr04_GetDistance(void);

#endif
