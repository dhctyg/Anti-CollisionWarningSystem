#include "buzzer.h"
#include "bsp.h"

void Buzzer_On(void)
{
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Off(void)
{
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

uint8_t Buzzer_Is_Echo(void)
{
    return GPIO_ReadOutputDataBit(BUZZER_PORT, BUZZER_PIN);
}

void Buzzer_Flash(uint32_t gap, uint32_t echo)
{
    /* static 变量在多次调用之间保留数值 */
    static uint8_t running = 0;
    static uint8_t is_on = 0;
    static uint32_t phase_start_ms = 0;
    static uint32_t phase_duration_ms = 0;

    uint32_t now;

    /* 鸣叫时间为 0：停止并复位状态 */
    if (echo == 0U)
    {
        Buzzer_Off();
        running = 0;
        is_on = 0;
        return;
    }

    /* 间隔为 0：持续鸣叫 */
    if (gap == 0U)
    {
        Buzzer_On();
        running = 0;
        is_on = 1;
        return;
    }

    now = BSP_GetUptime_ms();

    /* 首次启用：立即开始鸣叫 */
    if (running == 0U)
    {
        running = 1;
        is_on = 1;

        phase_start_ms = now;
        phase_duration_ms = echo;

        Buzzer_On();
        return;
    }

    /* 停鸣期间采用最新间隔，保留原来的计时起点 */
    if (is_on == 0U)
    {
        phase_duration_ms = gap;
    }

    /* 当前阶段时间未到 */
    if ((uint32_t)(now - phase_start_ms) < phase_duration_ms)
    {
        return;
    }

    /* 时间到，切换阶段，并锁定下一阶段的时长 */
    if (is_on != 0U)
    {
        Buzzer_Off();
        is_on = 0;
        phase_duration_ms = gap;
    }
    else
    {
        Buzzer_On();
        is_on = 1;
        phase_duration_ms = echo;
    }

    phase_start_ms = now;
}

/*
void Buzzer_Flash(uint32_t gap, uint32_t echo)
{
    uint32_t T = gap + echo;
    uint8_t is_echo;

    if (T == 0U)
    {
        Buzzer_Off();
        return;
    }

    is_echo = (BSP_GetUptime_ms() % T >= gap);

    if (is_echo != GPIO_ReadOutputDataBit(BUZZER_PORT, BUZZER_PIN))
    {
        GPIO_WriteBit(BUZZER_PORT, BUZZER_PIN,
                      is_echo ? Bit_SET : Bit_RESET);
    }
}
*/