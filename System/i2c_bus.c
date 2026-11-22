#include "i2c_bus.h"

/* 消息槽的结构体 */
typedef struct
{
    I2c_SlotStateTypeDef slot_state;
    uint8_t address;

    uint8_t msg[I2C_MSG_MAX];
    uint16_t length;
    uint16_t index;

    uint8_t is_read;
    uint8_t reg;

    uint32_t send_done_time;
} I2c_MsgSlotTypeDef;

/* 消息槽的初始化，可以通过 I2c_DeviceTypeDef 枚举访问 */
static I2c_MsgSlotTypeDef i2c_msg_slot[I2C_DEVICE_COUNT] = 
{
    {I2C_SLOT_IDLE, I2C_DEVICE_OLED_ADDRESS},
    {I2C_SLOT_IDLE, I2C_DEVICE_MPU6050_ADDRESS}
};



uint8_t I2cBus_SendReg(const I2c_SendRequestTypeDef *message)
{
    /* 检查消息槽是否空闲，若空闲则直接插入，返回 1 表示发送成功 */
    if (i2c_msg_slot[message.device].slot_state = I2C_SLOT_IDLE)
    {

    }
    /* 若为正在发送状态，延时后重试，若超时则返回 0 表示发送失败 */
    else
    {

    }
};

