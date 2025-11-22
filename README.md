# Irrig Smart - Projeto de Monitoramento e Controle de Irrigação

## Descrição

O **Irrig Smart** é um projeto desenvolvido para monitoramento e controle de sistemas de irrigação utilizando a placa Raspberry Pi Pico. Este sistema é capaz de visualizar e controlar o estado de irrigadores através de uma matriz de LEDs, além de exibir informações em um display OLED. O projeto também se conecta à internet para obter a hora atual via NTP e pode ser acessado através de uma interface web.

## Funcionalidades

- **Monitoramento em Tempo Real**: Através da matriz de LEDs, o sistema indica o estado de cada irrigador (desligado, seco, úmido, etc.).
- **Controle de Irrigação**: Permite ativar ou desativar irrigadores individualmente.
- **Interface Web**: Acesse o status dos irrigadores e altere seu estado via um navegador web.
- **Display OLED**: Exibe informações sobre o estado do sistema e o horário atual.
- **Conexão Wi-Fi**: O sistema se conecta a uma rede Wi-Fi para acesso remoto e sincronização de horário.

## Estrutura do Projeto

O projeto utiliza as seguintes bibliotecas e componentes:

- **Raspberry Pi Pico**: Placa microcontroladora.
- **WS2818B**: Controle de LEDs RGB.
- **SSD1306**: Display OLED para exibição de informações.
- **LWIP**: Biblioteca para comunicação de rede.
- **ADC**: Para leitura de valores do joystick.
- **I2C**: Para comunicação com o display OLED.

## Componentes Utilizados

- **LEDs RGB**: Utilizados para indicar o estado dos irrigadores.
- **Joystick**: Para navegação e seleção de irrigadores.
- **Botões**: Para controle manual.
- **Display OLED**: Para exibir informações em tempo real.
- **Raspberry Pi Pico**: Microcontrolador que integra todos os componentes.

## Como Funciona

1. **Inicialização**: O sistema inicializa todos os componentes, incluindo LEDs, display OLED e conexão Wi-Fi.
2. **Leitura do Joystick**: O sistema lê continuamente os valores do joystick para mover um cursor pela matriz de irrigadores.
3. **Atualização de Estado**: O estado de cada irrigador é atualizado com base nas entradas do joystick e nos botões.
4. **Interface Web**: O servidor HTTP permite que os usuários interajam com o sistema através de um navegador, onde podem visualizar e alterar o estado dos irrigadores.
5. **Sincronização de Horário**: O sistema obtém a hora atual de um servidor NTP para exibir no display OLED.

## Requisitos

- **Hardware**:
  - Raspberry Pi Pico
  - LEDs RGB WS2818B
  - Display OLED SSD1306
  - Joystick
  - Botões
  - Módulo Wi-Fi (integrado na Raspberry Pi Pico W)

- **Software**:
  - Ambiente de desenvolvimento para C/C++
  - Bibliotecas necessárias (disponíveis no repositório)

## Instalação

1. Clone o repositório:
   ```bash
   git clone https://github.com/seu_usuario/irrig-smart.git
   cd irrig-smart
