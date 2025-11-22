// aht10.c
#include "aht10.h"

// Inicializa o sensor AHT10
int aht10_init() {
    uint8_t cmd[3] = {AHT10_CMD_TRIGGER, 0x00, 0x00}; // Comando para iniciar o sensor
    i2c_write_blocking(i2c1, AHT10_ADDRESS, cmd, sizeof(cmd), false);
    sleep_ms(100); // Aguarda o sensor inicializar
    return 0; // Retorna 0 se a inicialização for bem-sucedida
}

// Lê a temperatura e umidade do AHT10
int aht10_read(float *temperatura, float *umidade) {
    uint8_t cmd[2] = {AHT10_CMD_READ, 0x00}; // Comando para ler os dados
    i2c_write_blocking(i2c1, AHT10_ADDRESS, cmd, sizeof(cmd), true);
    
    uint8_t data[6];
    i2c_read_blocking(i2c1, AHT10_ADDRESS, data, sizeof(data), false);

    // Verifica se a leitura foi bem-sucedida
    if (data[0] & 0x80) {
        // Extrai a umidade e a temperatura dos dados
        *umidade = ((data[1] << 8) | data[2]) * 100.0 / 65536.0; // Umidade em %
        *temperatura = (((data[3] & 0x7F) << 16) | (data[4] << 8) | data[5]) * 200.0 / 1048576.0 - 50.0; // Temperatura em °C
        return 0; // Retorna 0 se a leitura for bem-sucedida
    }
    return -1; // Retorna -1 se a leitura falhar
}