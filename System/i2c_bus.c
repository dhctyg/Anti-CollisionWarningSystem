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

    /* 发送请求变量 */
    uint8_t msg[I2C_MSG_MAX];

    /* 接收请求变量 */
    uint8_t reg;
    uint8_t read_data[I2C_MSG_MAX];
    uint16_t read_length;
    uint8_t is_enclosed;
} I2c_MsgSlotTypeDef;

/* 工作状态枚举 */
typedef enum
{
    I2C_WAIT_SEND_SB,
    I2C_WAIT_ADDR,
    I2C_WAIT_BTF,
    I2C_WAIT_RX,
    I2C_WAIT_STOP
} I2c_WorkStateTypeDef;

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
    /* 公共变量 */
    I2c_MsgSlotTypeDef *slot;
    /* 状态处理部分变量 */
    uint32_t primask_state;
    uint16_t remain_length;
    static I2c_WorkStateTypeDef work_state;
    /* 选槽部分变量 */
    static int device;
    /* 超时错误检测变量 */
    static uint32_t last_progress_time;
    static uint8_t timeout_check_state;
    static uint8_t time_update_mark;
    uint16_t errors;

    /* 选槽部分 */
    if(device == I2C_DEVICE_COUNT)
    {
        device = 0;
        return;
    }
    slot = &i2c_msg_slot[device];
    if(slot->slot_state == I2C_SLOT_IDLE)
    {
        device ++;
        return;
    }


    /* 超时与错误处理部分，不处于空闲状态时进入这里 */
    errors = (I2C_CHANNEL->SR1 & (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR));

    if(time_update_mark != 0U)
    {
        last_progress_time = BSP_GetUptime_ms();
        time_update_mark = 0U;
    }
    if(errors != 0U)
    {
        slot->slot_state = I2C_SLOT_ERROR;
    }
    if(I2C_SLOT_WAIT_TIMEOUT < (uint32_t)(BSP_GetUptime_ms() - last_progress_time) &&
       timeout_check_state != 0U)
    {
        slot->slot_state = I2C_SLOT_ERROR;
    }


    /* 状态处理 */
    if(timeout_check_state == 0U)
    {
        last_progress_time = BSP_GetUptime_ms();
        timeout_check_state = 1U;
    }
    switch(slot->slot_state)
    {
        case I2C_SLOT_IDLE:
            return;

        case I2C_SLOT_WAIT_SEND:
            /* 若 I2C 总线正在忙，返回等待空闲 */
            if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BUSY) == SET)
            {
                return;
            }

            /* 先写入 START 控制位，再修改状态 */
            I2C_GenerateSTART(I2C_CHANNEL, ENABLE);
            work_state = I2C_WAIT_SEND_SB;
            last_progress_time = BSP_GetUptime_ms();
            time_update_mark = 1U;
            slot->slot_state = I2C_SLOT_SENDING;

        case I2C_SLOT_SENDING:
            switch(work_state)
            {
                case I2C_WAIT_SEND_SB:
                    /* START 尚未发送完成，继续等待 SB */
                    if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_SB) == RESET)
                    {
                        return;
                    }

                    /* 向总线发送地址 */
                    I2C_Send7bitAddress(I2C_CHANNEL,
                                        (uint8_t)(slot->address << 1),
                                        I2C_Direction_Transmitter);
                    time_update_mark = 1U;
                    work_state = I2C_WAIT_ADDR;
                    return;
                
                case I2C_WAIT_ADDR:
                    /* ADDR 未置位，继续等待 */
                    if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_ADDR) == RESET)
                    {
                        return;
                    }

                    /* ADDR 置位，通过读取寄存器方式自动清除 ADDR 标志位 */
                    (void)I2C_CHANNEL->SR1;
                    (void)I2C_CHANNEL->SR2;

                    /* 判断读写操作，执行下一步操作 */
                    if(slot->is_read != 0U)
                    {
                        /* 读请求先写寄存器号，再等待接受 */
                        I2C_SendData(I2C_CHANNEL, slot->reg);
                    }
                    else
                    {
                        /* 先发送 1 个字节，index 更新指向下一个字节 */
                        I2C_SendData(I2C_CHANNEL, slot->msg[slot->index]);
                        slot->index ++;
                    }
                    time_update_mark = 1U;
                    work_state = I2C_WAIT_BTF;
                    return;

                case I2C_WAIT_BTF:
                    if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BTF) == RESET)
                    {
                        return;
                    }

                    if(slot->is_read != 0U)
                    {
                        /* 读数据状态用重复发送 START 保持总线所有权 */
                        I2C_GenerateSTART(I2C_CHANNEL, ENABLE);
                        work_state = I2C_WAIT_SEND_SB;
                        time_update_mark = 1U;
                        slot->slot_state = I2C_SLOT_WAIT_RECEIVE;
                        return;
                    }

                    if(slot->index < slot->length)
                    {
                        I2C_SendData(I2C_CHANNEL, slot->msg[slot->index]);
                        time_update_mark = 1U;
                        slot->index ++;
                        return;
                    }

                    /* 传输完成 */
                    I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
                    time_update_mark = 1U;
                    work_state = I2C_WAIT_STOP;
                    return;

                case I2C_WAIT_STOP:
                    /* 检查 CR1 寄存器 STOP 标志位状态 */
                    if((I2C_CHANNEL->CR1 & I2C_CR1_STOP) != 0U)
                    {
                        return;
                    }
                    /* 若停止位置位，则记录时间并修改状态 */
                    device ++;
                    timeout_check_state = 0U;
                    slot->slot_state = I2C_SLOT_IDLE; 

                default:
                    return;     
            }

        case I2C_SLOT_WAIT_RECEIVE:
            switch(work_state)
            {
                case I2C_WAIT_SEND_SB:
                    /* 检查 START 是否发出 */
                    if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_SB) == RESET)
                    {
                        return;
                    }
                    /* 若读取长度 2 字节，设置 POS */
                    if(slot->length == 2U)
                    {
                        I2C_NACKPositionConfig(I2C_CHANNEL, I2C_NACKPosition_Next);
                    }
                    /* 以读方向发送从机地址 */
                    I2C_Send7bitAddress(I2C_CHANNEL,
                                        (uint8_t)(slot->address << 1),
                                        I2C_Direction_Receiver);
                    /* 更新状态 */
                    time_update_mark = 1U;
                    work_state = I2C_WAIT_ADDR;
                    return;

                case I2C_WAIT_ADDR:
                    if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_ADDR) == RESET)
                    {
                        return;
                    }
                    /* 暂时关闭中断，对单字节和双字节情况特殊配置 */
                    primask_state = __get_PRIMASK();
                    __disable_irq();

                    if(slot->length == 1U)
                    {
                        I2C_AcknowledgeConfig(I2C_CHANNEL, DISABLE);
                    }

                    (void)I2C_CHANNEL->SR1;
                    (void)I2C_CHANNEL->SR2;

                    if(slot->length == 1U)
                    {
                        /* 先请求 STOP，下一阶段取数据 */
                        I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
                    }

                    if(slot->length == 2U)
                    {
                        I2C_AcknowledgeConfig(I2C_CHANNEL, DISABLE);
                    }
                    __set_PRIMASK(primask_state);

                    /* 更新状态 */
                    time_update_mark = 1U;
                    work_state = I2C_WAIT_RX;
                    slot->slot_state = I2C_SLOT_RECEIVING;
                    return;

                default:
                    return;
            }
        case I2C_SLOT_RECEIVING:
            switch(work_state)
            {
                case I2C_WAIT_RX:
                    remain_length = slot->length - slot->index;
                    /* 剩余字节数大于 3 时正常读取数据 */
                    if(remain_length > 3U)
                    {
                        if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BTF) == RESET)
                        {
                            return;
                        }

                        /* 读取一位数据后更新 index */
                        slot->msg[slot->index] = I2C_ReceiveData(I2C_CHANNEL);
                        slot->index ++;
                        time_update_mark = 1U;
                        return;
                    }
                    /* 剩余字节数等于 3，需要修改 NACK 标志位，以结束读取 */
                    else if(remain_length == 3U)
                    {
                        if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BTF) == RESET)
                        {
                            return;
                        }

                        primask_state = __get_PRIMASK();
                        __disable_irq();
                        I2C_AcknowledgeConfig(I2C_CHANNEL, DISABLE);
                        slot->msg[slot->index] = I2C_ReceiveData(I2C_CHANNEL);
                        slot->index ++;
                        /* 此时最后一位数据已进入寄存器，发送停止请求，继续读取下一位使剩余字节为 1 */
                        I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
                        slot->msg[slot->index] = I2C_ReceiveData(I2C_CHANNEL);
                        slot->index ++;
                        __set_PRIMASK(primask_state);
                        time_update_mark = 1U;
                        return;
                    }
                    else if(remain_length == 2U)
                    {
                        if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_BTF) == RESET)
                        {
                            return;
                        }
                        primask_state = __get_PRIMASK();
                        __disable_irq();
                        I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
                        slot->msg[slot->index] = I2C_ReceiveData(I2C_CHANNEL);
                        slot->index ++;
                        __set_PRIMASK(primask_state);
                        time_update_mark = 1U;
                        return;
                    }
                    else if(remain_length == 1U)
                    {
                        if(I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_RXNE) == RESET)
                        {
                            return;
                        }

                        slot->msg[slot->index] = I2C_ReceiveData(I2C_CHANNEL);
                        /* 全部读取完，index 归零 */
                        slot->index = 0U;
                        slot->is_enclosed = 0U;
                        time_update_mark = 1U;
                        work_state = I2C_WAIT_STOP;
                        return;
                    }
                    else
                    {
                        return;
                    }

                case I2C_WAIT_STOP:
                    if((I2C_CHANNEL->CR1 & I2C_CR1_STOP) != 0U)
                    {
                        /* 等待 STOP 置位 */
                        return;
                    }
                    I2C_NACKPositionConfig(I2C_CHANNEL, I2C_NACKPosition_Current);
                    I2C_AcknowledgeConfig(I2C_CHANNEL, ENABLE);

                    device ++;
                    memcpy(slot->read_data, slot->msg, slot->length);
                    slot->read_length = slot->length;
                    timeout_check_state = 0U;
                    slot->slot_state = I2C_SLOT_IDLE;
                    return;

                default:
                    return;
            }

        case I2C_SLOT_ERROR:
            if(errors == I2C_SR1_AF &&
               (I2C_CHANNEL->CR1 & (I2C_CR1_START | I2C_CR1_STOP)) == 0U &&
               I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_MSL) == SET)
            {
                /* 应答失败且 START 和 STOP 均未置位且处于主机状态，请求 STOP 并清除标志位 */
                I2C_GenerateSTOP(I2C_CHANNEL, ENABLE);
                I2C_ClearFlag(I2C_CHANNEL, I2C_FLAG_AF);
                /* 等待 STOP 完成，同时检测超时 */
                last_progress_time = BSP_GetUptime_ms();
                return;
            }
            if(errors == 0U &&
               (I2C_CHANNEL->CR1 & I2C_CR1_STOP) != 0U &&
               (uint32_t)(BSP_GetUptime_ms() - last_progress_time) < I2C_SLOT_WAIT_TIMEOUT)
            {
                /* 等待 STOP 置位 */
                return;
            }
            if (errors != 0U ||
               (I2C_CHANNEL->CR1 & (I2C_CR1_START | I2C_CR1_STOP)) != 0U ||
               I2C_GetFlagStatus(I2C_CHANNEL, I2C_FLAG_MSL) == SET)
            {
                /* 存在错误或控制操作未完成或处于主机状态，强制复位 */
                BSP_I2cReset();
            }
            else
            {
                I2C_NACKPositionConfig(I2C_CHANNEL, I2C_NACKPosition_Current);
                I2C_AcknowledgeConfig(I2C_CHANNEL, ENABLE);
            }

            slot->index = 0U;
            slot->slot_state = I2C_SLOT_IDLE;
            timeout_check_state = 0U;
            time_update_mark = 0U;
            device++;
            return;
        }
}

uint8_t I2cBus_GetReadMsg(I2c_DeviceTypeDef device, uint8_t *buffer)
{
    I2c_MsgSlotTypeDef *slot;

    if(device >= I2C_DEVICE_COUNT || buffer == NULL)
    {
        return 0U;
    }

    slot = &i2c_msg_slot[device];

    if(slot->is_enclosed == 0U && slot->read_length != 0U)
    {
        memcpy(buffer, slot->read_data, slot->read_length);
        slot->is_enclosed = 1U;
        return 1U;
    }
    else
    {
        return 0U;
    }

}
