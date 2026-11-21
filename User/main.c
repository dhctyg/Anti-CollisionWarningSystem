#include "bsp.h"
#include "buzzer.h"
#include "hcsr04.h"

int main(void)
{
	/* 可在调试器中观察距离（cm）和停鸣间隔（ms）。 */
	volatile uint32_t distance_cm = 0xFFFFFFFFU;
	volatile uint32_t gap_ms = 0U;

	BSP_Init();

	while(1)
	{
		Hcsr04_TaskHandle();
		distance_cm = Hcsr04_GetDistance();

		/* 无效距离、低于量程下限或超过 200 cm 时静音。 */
		if (distance_cm < 2U || distance_cm > 200U)
		{
			gap_ms = 0U;
			Buzzer_Off();
		}
		else
		{
			/* 每次鸣叫 50 ms；距离越近，停鸣间隔越短。 */
			if (distance_cm <= 10U)
			{
				gap_ms = 50U;
			}
			else
			{
				/* 10~200 cm 对应 50~950 ms 的停鸣间隔。 */
				gap_ms = 50U + (distance_cm - 10U) * 900U / 190U;
			}

			Buzzer_Flash(gap_ms, 50U);
		}
	}
}
