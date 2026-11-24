#include "i2c_bus.h"

/* 消息槽的结构体 */
typedef struct
{
    /* 共用变量 */
    I2c_SlotStateTypeDef slot_state;
    uint8_t address;
    uint16_t length;    /* 此变量在写请求中表示 msg 长度，在接收请求中表示要读的字节数 */
    uint16_t index;
    uint8_t is_read;
    uint32_t send_done_time;

    /* 发送请求变量 */
    uint8_t msg[I2C_MSG_MAX];

    /* 接收请求变量 */
    uint8_t reg;
    uint8_t read_data[I2C_MSG_MAX];
    uint16_t read_length;
    uint32_t read_done_time;
} I2c_MsgSlotTypeDef;

/* 消息槽的初始化，可以通过 I2c_DeviceTypeDef 枚举访问 */
static I2c_MsgSlotTypeDef i2c_msg_slot[I2C_DEVICE_COUNT] = 
{
    {I2C_SLOT_IDLE, I2C_ADDRESS_OLED},
    {I2C_SLOT_IDLE, I2C_ADDRESS_MPU6050}
};

/* 发送消息的入槽函数 */
uint8_t I2cBus_SendReg(const I2c_SendRequestTypeDef *message)
{
    I2c_MsgSlotTypeDef *slot = NULL;

    /* message 结构体与其中 msg 变量是否为空的检查 */
    if(message == NULL || message->msg == NULL)
    {
        return 0U;
    }
    /* device 变量与 length 是否合法的检查 */
    if((unsigned int)message->device >= I2C_DEVICE_COUNT ||
       message->length == 0U ||
       message->length > I2C_MSG_MAX)
    {
        return 0U;
    }
    
    /* 将槽的地址传给 slot，便于后续处理 */
    slot = &i2c_msg_slot[message->device];

    /* 检查槽的状态是否空闲，若不处于空闲状态，返回失败 */
    if(slot->slot_state != I2C_SLOT_IDLE)
    {
        return 0U;
    }

    /* 数据入槽 */
    memcpy(slot->msg, message->msg, message->length);
    slot->length = message->length;
    slot->index = 0U;
    slot->is_read = 0U;

    /* 槽状态更新并返回成功 */
    slot->slot_state = I2C_SLOT_WAIT_SEND;
    return 1U;
};

/* 读取消息的入槽函数 */
uint8_t I2cBus_ReadReg(const I2c_ReadRequestTypeDef *message)
{
    I2c_MsgSlotTypeDef *slot = NULL;

    /* 状态异常检测 */
    if(message == NULL ||
       (unsigned int)message->device >= I2C_DEVICE_COUNT ||
       message->length == 0U ||
       message->length > I2C_MSG_MAX)
    {
        return 0U;
    }

    /* 将槽的地址传给 slot，便于后续处理 */
    slot = &i2c_msg_slot[message->device];

    /* 检查槽的状态是否空闲，若不处于空闲状态，返回失败 */
    if(slot->slot_state != I2C_SLOT_IDLE)
    {
        return 0U;
    }

    /* 数据入槽 */
    slot->reg = message->reg;
    slot->length = message->length;
    slot->index = 0U;
    slot->is_read = 1U;

    /* 槽状态更新并返回成功 */
    slot->slot_state = I2C_SLOT_WAIT_SEND;
    return 1U;
}

/* 槽任务处理函数 */
void I2cBus_TaskHandle(void)
{

}