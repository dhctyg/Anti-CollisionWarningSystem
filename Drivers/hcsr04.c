#include "bsp.h"
#include "hcsr04.h"

static volatile Hcsr04_WorkStateTypeDef hcsr04_state = HCSR04_GAPTIME;
static volatile Hcsr04_ResultStateTypeDef hcsr04_result_state = HCSR04_RESULT_WRONG;
static volatile uint32_t start_measure_time;
static volatile uint32_t start_gap_time;
static volatile uint16_t rising_time_us;
static volatile uint16_t falling_time_us;
static volatile uint32_t distance_cm = 0xFFFFFFFFU;

/* us 级延时使用阻塞函数，防止定时不正确 */
static void Hcsr04_Delay_Us(uint16_t delay_time_us)
{
    uint16_t start_time;
    start_time = TIM_GetCounter(HCSR04_TIM);

    while ((uint16_t)(TIM_GetCounter(HCSR04_TIM) - start_time) < delay_time_us)
    {
        /* 阻塞 */
    }
}

static void Hcsr04_SendTrigger(void)
{
    /* 清除 TIM 标志位 */
    TIM_ClearFlag(HCSR04_TIM,
                  HCSR04_TIM_FLAG_IF_RISING | HCSR04_TIM_FLAG_IF_FALLING |
                      HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING);

    /* 完成 TRIG 脉冲发送 */
    GPIO_ResetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
    Hcsr04_Delay_Us(2);

    GPIO_SetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
    Hcsr04_Delay_Us(12);

    GPIO_ResetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
}

void Hcsr04_IrqHandle(void)
{
    uint16_t sr_status;
    uint32_t measured_cm;

    /* 只有处于 echo 脉冲测量状态且处于等待结果时才进行中断处理 */
    if ((hcsr04_state == HCSR04_WAITING_ECHO) &&
        (hcsr04_result_state == HCSR04_WAITING_RISING || hcsr04_result_state == HCSR04_WAITING_FALLING))
    {
        /* 读取 TIM 状态寄存器 */
        sr_status = (uint16_t)HCSR04_TIM->SR;

        /* 溢出标志位均为 0 ，否则为错误输出，错误处理给 TaskHandle 函数处理 */
        if ((sr_status & (HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING)) == 0U)
        {
            /* 上升沿捕获标志位为 1 时，先检查是否为等待上升沿，否则抛出错误结果 */
            if ((sr_status & HCSR04_TIM_FLAG_IF_RISING) != 0U)
            {
                /* 是否为等待上升沿的判断 */
                if (hcsr04_result_state != HCSR04_WAITING_RISING)
                {
                    hcsr04_result_state = HCSR04_RESULT_WRONG;
                    TIM_ClearFlag(HCSR04_TIM,
                                  HCSR04_TIM_FLAG_IF_RISING | HCSR04_TIM_FLAG_IF_FALLING |
                                      HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING);
                    return;
                }
                rising_time_us = HCSR04_RISING_CAPTURE(HCSR04_TIM);
                hcsr04_result_state = HCSR04_WAITING_FALLING;
            }

            /* 下降沿捕获标志位为 1 时，先检查是否为等待下降沿，否则抛出错误结果 */
            if ((sr_status & HCSR04_TIM_FLAG_IF_FALLING) != 0U)
            {
                if (hcsr04_result_state != HCSR04_WAITING_FALLING)
                {
                    hcsr04_result_state = HCSR04_RESULT_WRONG;
                    TIM_ClearFlag(HCSR04_TIM,
                                  HCSR04_TIM_FLAG_IF_RISING | HCSR04_TIM_FLAG_IF_FALLING |
                                      HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING);
                    return;
                }
                falling_time_us = HCSR04_FALLING_CAPTURE(HCSR04_TIM);

                /* 测距结果计算 */
                measured_cm = ((uint32_t)((uint16_t)(falling_time_us - rising_time_us)) * 17U + 500U) / 1000U;
                /* distance 异常检查 */
                if (measured_cm < 2U || measured_cm > 400U)
                {
                    hcsr04_result_state = HCSR04_RESULT_WRONG;
                    return;
                }
                distance_cm = measured_cm;
                hcsr04_result_state = HCSR04_RESULT_DONE;
            }
        }
        else
        {
            hcsr04_result_state = HCSR04_RESULT_WRONG;
            TIM_ClearFlag(HCSR04_TIM,
                          HCSR04_TIM_FLAG_IF_RISING | HCSR04_TIM_FLAG_IF_FALLING |
                              HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING);
        }
    }
    else
    {
        TIM_ClearFlag(HCSR04_TIM,
            HCSR04_TIM_FLAG_IF_RISING |
            HCSR04_TIM_FLAG_IF_FALLING |
            HCSR04_TIM_FLAG_OF_RISING |
            HCSR04_TIM_FLAG_OF_FALLING);
    }
}

void Hcsr04_TaskHandle(void)
{
    uint32_t primask_state;

    switch (hcsr04_state)
    {
    /* 状态 1 ：空闲状态。此时发送 Trig 脉冲，启动测量计时器 */
    case HCSR04_IDLE:
        /* 中断屏蔽 */
        primask_state = __get_PRIMASK();
        __disable_irq();

        hcsr04_result_state = HCSR04_WAITING_RISING;
        hcsr04_state = HCSR04_WAITING_ECHO;
        start_measure_time = BSP_GetUptime_ms();

        Hcsr04_SendTrigger();

        __set_PRIMASK(primask_state);
        break;

    /* 状态 2 ：等待测距脉冲，此时检查是否超时或错误结果，若发生进入错误处理 */
    case HCSR04_WAITING_ECHO:
        /* 超时检测 */
        if (HCSR04_TIMEOUT_MS <= (uint32_t)(BSP_GetUptime_ms() - start_measure_time))
        {
            primask_state = __get_PRIMASK();
            __disable_irq();
            hcsr04_result_state = HCSR04_RESULT_WRONG;
            hcsr04_state = HCSR04_GAPTIME;
            __set_PRIMASK(primask_state);
        }
        /* 输出处理 */
        if (hcsr04_result_state == HCSR04_RESULT_WRONG)
        {
            TIM_ClearFlag(HCSR04_TIM,
                          HCSR04_TIM_FLAG_IF_RISING | HCSR04_TIM_FLAG_IF_FALLING |
                          HCSR04_TIM_FLAG_OF_RISING | HCSR04_TIM_FLAG_OF_FALLING);
            primask_state = __get_PRIMASK();
            __disable_irq();
            hcsr04_state = HCSR04_GAPTIME;
            __set_PRIMASK(primask_state);
        }
        else if (hcsr04_result_state == HCSR04_RESULT_DONE)
        {
            primask_state = __get_PRIMASK();
            __disable_irq();
            start_gap_time = BSP_GetUptime_ms();
            hcsr04_state = HCSR04_GAPTIME;
            __set_PRIMASK(primask_state);
        }
        else
        {
            return;
        }
        break;

    /* 状态 3 ：测量间隔计时 */
    case HCSR04_GAPTIME:
        if (HCSR04_GAP_MS <= (uint32_t)(BSP_GetUptime_ms() - start_gap_time))
        {
            hcsr04_state = HCSR04_IDLE;
        }
        break;
    }
}

uint32_t Hcsr04_GetDistance(void)
{
    return distance_cm;
}
