#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>
#include <string.h> /* 引入 memcpy 函数 */
#include "bsp.h"

#define I2C_SLOT_WAIT_TIMEOUT 5U
#define I2C_MSG_MAX 32U
#define I2C_DEVICE_COUNT 2U

/* 从设备列表 */
typedef enum
{
    I2C_DEVICE_OLED = 0x3C,
    I2C_DEVICE_MPU6050 = 0x68,
} I2c_DeviceIDTypeDef;

/* 消息状态列表 */
typedef enum
{
    I2C_SLOT_IDLE,
    I2C_SLOT_WAIT_SEND,
    I2C_SLOT_SENDING,
    I2C_SLOT_WAIT_REPLY,
    I2C_SLOT_DONE,
    I2C_SLOT_ERROR
} I2c_SlotStateTypeDef;

/* 发送消息结构体 */
typedef struct
{
    I2c_DeviceTypeDef device;
    uint8_t address;
    const uint8_t *msg;
    uint16_t length;
} I2c_SendRequestTypeDef;

/* 接收消息结构体 */
typedef struct
{
    I2c_DeviceTypeDef device;
    uint8_t address;
    uint8_t reg;
} I2c_ReadRequestTypeDef;

/* TaskHandle 函数主要用于槽内消息的发送与状态更新以及错误处理 */
void I2cBus_TaskHandle(void);
/* SendReg 和 ReadReg 函数主要用于向槽内更新信息 */
uint8_t I2cBus_SendReg(const I2c_SendRequestTypeDef *message);
uint8_t I2cBus_ReadReg(const I2c_SendRequestTypeDef *message);
/* GetReadMsg 用于读取最近一次向某设备读取的数据 */
uint16_t I2cBus_GetReadMsg(uint8_t address, uint8_t *buffer);
/* GetMsgTime 用于读取最近一次读取数据更新的时间，用于判断两次读取的数据是否相同，还应包含相同值的二次判断 */
uint32_t I2cBus_GetMsgTime(uint8_t address);

#endif
