// aht10.h
#ifndef AHT10_H
#define AHT10_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"

// Endereço I2C do AHT10
#define AHT10_ADDRESS 0x38

// Comandos do AHT10
#define AHT10_CMD_TRIGGER 0xAC
#define AHT10_CMD_READ 0x33
#define AHT10_CMD_SOFT_RESET 0xBA

// Funções para inicializar e ler do AHT10
int aht10_init();
int aht10_read(float *temperatura, float *umidade);

#endif // AHT10_H