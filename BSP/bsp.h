#ifndef BSP_H
#define BSP_H
#include "stm32f10x.h"
#include <stdint.h> 

//蜂鸣器引脚定义
#define BUZZER_RCC_PORT                 RCC_APB2Periph_GPIOB
#define BUZZER_PORT                     GPIOB
#define BUZZER_PIN                      GPIO_Pin_0

//定时器定义
#define TIM_RCC_PORT                    RCC_APB1Periph_TIM2

//HC-SR04引脚定义
#define HCSR04_TRIG_RCC_PORT            RCC_APB2Periph_GPIOA
#define HCSR04_TRIG_PORT                GPIOA
#define HCSR04_TRIG_PIN                 GPIO_Pin_1
#define HCSR04_ECHO_RCC_PORT            RCC_APB2Periph_GPIOB
#define HCSR04_ECHO_RCC_AF_PORT         RCC_APB2Periph_AFIO
#define HCSR04_ECHO_PORT                GPIOB
#define HCSR04_ECHO_PIN                 GPIO_Pin_10

//HC-SR04定时器配置
#define HCSR04_GPIO_REMAP_CONFIG        GPIO_PartialRemap2_TIM2
#define HCSR04_TIM                      TIM2
#define HCSR04_TIM_IC_RISING_CHANNEL    TIM_Channel_3
#define HCSR04_TIM_IC_FALLING_CHANNEL   TIM_Channel_4

//HC-SR04定时器中断配置
#define HCSR04_TIM_IRQ_CHANNEL          TIM2_IRQn
#define HCSR04_TIM_IT_RISING_CHANNEL    TIM_IT_CC3
#define HCSR04_TIM_IT_FALLING_CHANNEL   TIM_IT_CC4
#define HCSR04_TIM_FLAG_IF_RISING       TIM_FLAG_CC3
#define HCSR04_TIM_FLAG_IF_FALLING      TIM_FLAG_CC4
#define HCSR04_TIM_FLAG_OF_RISING       TIM_FLAG_CC3OF
#define HCSR04_TIM_FLAG_OF_FALLING      TIM_FLAG_CC4OF
#define HCSR04_RISING_CAPTURE(TIMx)     TIM_GetCapture3(TIMx)
#define HCSR04_FALLING_CAPTURE(TIMx)    TIM_GetCapture4(TIMx)

//I2C总线初始化
#define I2C_RCC_GPIO_PORT               RCC_APB2Periph_GPIOB
#define I2C_RCC_PORT                    RCC_APB1Periph_I2C1
#define I2C_CHANNEL                     I2C1
#define I2C_PORT                        GPIOB
#define I2C_SCL_PIN                     GPIO_Pin_6
#define I2C_SDA_PIN                     GPIO_Pin_7


void BSP_Init(void);
uint32_t BSP_GetUptime_ms(void);

//系统上电后的毫秒计数，定义在 stm32f10x_it.c 的 SysTick_Handler 中，由 BSP_Init 启动
extern volatile uint32_t uptime_ms;

#endif
