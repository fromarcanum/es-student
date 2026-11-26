#pragma once

enum
{
    SLAVE_I2C_ADDRESS = 0x42,
    SLAVE_REG_CMD = 0x00,   // Write only: 'e' enables the LED, 'd' disables it
    SLAVE_REG_STATE = 0x01  // Read/write: 0 off, 1 on
};

void slave_device_init(void);
void slave_device_handle(void);
