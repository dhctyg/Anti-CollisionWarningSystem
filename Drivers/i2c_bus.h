#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdint.h>

typedef enum
{
    I2C_IDEL,
    I2C_SENDING
} I2c_StateTypeDef;

uint8_t I2cBus_Send(uint8_t address, const uint8_t *msg, uint16_t length);

#endif