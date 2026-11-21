/**
  ******************************************************************************
  * @file    bsp.c
  * @date    2026-11-13
  * @brief   板级支持包
  *
  * @details
  *   负责 STM32 板级外设的初始化：
  *   - GPIO 引脚初始化
  *   - SysTick 毫秒计时器初始化
  *   - TIM2 微秒计时器初始化（待实现）
  ******************************************************************************
  */

#include "bsp.h"
#include "stm32f10x.h"


/**
 * @brief   蜂鸣器初始化函数
 */
static void BSP_BuzzerInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(BUZZER_RCC_PORT, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = BUZZER_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);    //上电输出低电平
}

/**
 * @brief   TIM2计时器初始化函数
 */
static void BSP_TIM2Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructInit(&TIM_InitStructure);

    TIM_InitStructure.TIM_Prescaler = 71;
    TIM_InitStructure.TIM_Period = 65535;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    TIM_Cmd(TIM2, ENABLE);

}

/**
 * @brief   HC-SR04初始化函数
 */
static void BSP_Hcsr04Init(void)
{
/* 结构体变量初始化 -------------------------------------------------------------*/
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_ICInitTypeDef TIM_ICStructure;              //定时器捕获初始化
    NVIC_InitTypeDef NVIC_InitStructure;            //中断初始化
/* 时钟 -----------------------------------------------------------------------*/
    RCC_APB2PeriphClockCmd(
        HCSR04_TRIG_RCC_PORT |
        HCSR04_ECHO_RCC_PORT |
        HCSR04_ECHO_RCC_AF_PORT,
        ENABLE);
    RCC_APB1PeriphClockCmd(TIM_RCC_PORT, ENABLE);
/* TRIG 初始化 -----------------------------------------------------------------*/
    GPIO_ResetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);

    GPIO_InitStructure.GPIO_Pin = HCSR04_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(HCSR04_TRIG_PORT, &GPIO_InitStructure);
/* TIM2 重映射到 PB10 以及 ECHO 初始化 -------------------------------------------*/
    GPIO_PinRemapConfig(HCSR04_GPIO_REMAP_CONFIG, ENABLE);
    GPIO_InitStructure.GPIO_Pin = HCSR04_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(HCSR04_ECHO_PORT, &GPIO_InitStructure);
/* TIM2 初始化 -----------------------------------------------------------------*/
    TIM_TimeBaseStructInit(&TIM_InitStructure);
    TIM_InitStructure.TIM_Prescaler = 71;
    TIM_InitStructure.TIM_Period = 65535;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
//输入捕获初始化
    TIM_ICStructure.TIM_Channel = HCSR04_TIM_IC_RISING_CHANNEL;
    TIM_ICStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICStructure.TIM_ICFilter = 0;
    TIM_ICInit(HCSR04_TIM, &TIM_ICStructure);

    TIM_ICStructure.TIM_Channel = HCSR04_TIM_IC_FALLING_CHANNEL;
    TIM_ICStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
    TIM_ICStructure.TIM_ICSelection = TIM_ICSelection_IndirectTI;
    TIM_ICInit(HCSR04_TIM, &TIM_ICStructure);
//TIM2中断初始化
    NVIC_InitStructure.NVIC_IRQChannel = HCSR04_TIM_IRQ_CHANNEL;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ClearITPendingBit(HCSR04_TIM, HCSR04_TIM_IT_RISING_CHANNEL | HCSR04_TIM_IT_FALLING_CHANNEL);
    TIM_ITConfig(HCSR04_TIM, HCSR04_TIM_IT_RISING_CHANNEL | HCSR04_TIM_IT_FALLING_CHANNEL, ENABLE);

    TIM_Cmd(HCSR04_TIM, ENABLE);
}

static void BSP_I2CInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB2PeriphClockCmd(I2C_RCC_GPIO_PORT, ENABLE);
    RCC_APB1PeriphClockCmd(I2C_RCC_PORT, ENABLE);

    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);

    I2C_DeInit(I2C_CHANNEL);

    I2C_StructInit(&I2C_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 100000U;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Init(I2C_CHANNEL, &I2C_InitStructure);

}

static void BSP_OledInit(void)
{

}

static void BSP_Mpu6050Init(void)
{
    
}

void BSP_Init(void)
{
    //定时器初始化成功检测
    SystemCoreClockUpdate();    //更新 SystemCoreClock 变量
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)  //配置 SysTick 每 1ms 更新一次
    {
        while(1)
        {
            //定时器初始化失败的阻塞函数
        }
    }

    //蜂鸣器初始化
    BSP_BuzzerInit();
    BSP_Hcsr04Init();
}


uint32_t BSP_GetUptime_ms(void)
{
    return uptime_ms;
}
