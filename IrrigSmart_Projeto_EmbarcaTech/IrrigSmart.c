/* Irrig Smart - Praça das Flores - UFERSA - Campus Angicos/RN
Marcelo Vitorino Dantas Júnior - Matricula: 202421511720291
*/
// Todas as bibliotecas utilizadas no projeto
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
#include "hardware/pwm.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "pico/cyw43_driver.h"
#include <stdlib.h>
#include <ctype.h>
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "stdbool.h"
#include <time.h>
#include "lwip/apps/http_client.h"
#include "aht10.h"

// Definições de Wi-Fi
#define WIFI_SSID "PROXXIMA-178171_5G" // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_SENHA "17041991"     // Substitua pela senha da sua rede Wi-Fi

// Definição do número de LEDs e pino.
#define PINO_LED 7
#define LED_VERMELHO 13
#define LED_VERDE 11
#define LED_AZUL 12
#define LEDS 25

// Definições do ThingSpeak
#define THINGSPEAK_API_KEY_WRITE "AAWGP54X0VJNT3KJ"
#define THINGSPEAK_API_KEY_READ "GPHU5U7W7ZY3JMC4"
#define THINGSPEAK_CHANNEL_ID "2839984"
#define THINGSPEAK_URL "api.thingspeak.com"

// Definições dos pinos para o joystick e botão
#define PINO_VRX 26    // Define o pino GP26 para o eixo X do joystick (Canal ADC0).
#define PINO_VRY 27    // Define o pino GP27 para o eixo Y do joystick (Canal ADC1).
#define PINO_SW 22     // Define o pino GP22 para o botão do joystick (entrada digital).
#define PINO_BOTAO_A 5 // Define o pino GP5 para o botão A (entrada digital).
#define PINO_BOTAO_B 6 // Define o pino GP6 para o botão B (entrada digital).

// Valores de referência para o estado neutro do joystick
#define ZONA_MORTA 200               // Define uma zona morta para evitar oscilações pequenas
#define CENTRO_X 2048                // Centro teórico do ADC (meio de 0-4095)
#define CENTRO_Y 2048                // Centro teórico do ADC (meio de 0-4095)
#define LIMIAR 1000                  // Limiar para detectar um movimento significativo
#define INTERVALO_ATUALIZACAO 200000 // Intervalo em microssegundos (200ms)

// Conexão I2C para a tela
#define I2C_SDA 14
#define I2C_SCL 15

// Variáveis globais
time_t tempo_inicial;                // Variável para armazenar o tempo inicial
uint32_t ultima_atualizacao_relogio; // Variável para armazenar o tempo da última atualização do relógio

void atualizar_buffer_led();
void limpar_leds();
void escrever_leds();
int mudar_estado_led(int endereco_led_x, int endereco_led_y, int estado);
void inicializar_leds(uint pino);
void gerar_string_status(char *buffer);
void processar_requisicao(char *req, struct tcp_pcb *tpcb);
void exibir(char *texto, int linha);

// Funções para controlar a matriz de LEDs
bool flag_led = true;          // Flag para ligar/desligar a matriz dos LEDs
bool flag_selecao = false;     // Flag para selecionar o LED
bool piscar = 1;               // Controlar estado do seletor
int r, g, b = 0;               // Variáveis para controlar a cor do LED
int matriz_led[5][5][3] = {0}; // Matriz para organizar a cor dos LEDs
int matriz_estado[5][5] = {0}; // Matriz para guardar o estado de cada ponto

struct pixel_t
{                    // Definição de pixel GRB
    uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};

typedef struct pixel_t pixel_t; // Definição de pixel_t como um tipo de dado.
typedef pixel_t npLED_t;        // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.
npLED_t leds[LEDS];             // Declaração do buffer de pixels que formam a matriz_led.
PIO pio_np;                     // Variáveis para uso da máquina PIO.
uint sm;                        // Variáveis para uso da máquina PIO.

void inicializar_leds(uint pino)
{ // Inicializa a máquina PIO para controle da matriz_led de LEDs.
    // Cria programa PIO.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    pio_np = pio0;

    // Toma posse de uma máquina PIO.
    sm = pio_claim_unused_sm(pio_np, false);
    if (sm < 0)
    {
        pio_np = pio1;
        sm = pio_claim_unused_sm(pio_np, true); // Se nenhuma máquina estiver livre, panic!
    }

    // Inicia programa na máquina PIO obtida.
    ws2818b_program_init(pio_np, sm, offset, pino, 800000.f);

    // Limpa buffer de pixels.
    for (uint i = 0; i < LEDS; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void definir_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b)
{ // Atribui uma cor RGB a um LED.
    leds[indice].R = r;
    leds[indice].G = g;
    leds[indice].B = b;
}

void limpar_leds()
{ // Limpa o buffer de pixels.
    for (uint i = 0; i < LEDS; ++i)
        definir_led(i, 0, 0, 0);
    memset(matriz_led, 0, sizeof(matriz_led));
}

void escrever_leds()
{ // Escreve os dados do buffer nos LEDs.
    // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
    for (uint i = 0; i < LEDS; ++i)
    {
        pio_sm_put_blocking(pio_np, sm, leds[i].G);
        pio_sm_put_blocking(pio_np, sm, leds[i].R);
        pio_sm_put_blocking(pio_np, sm, leds[i].B);
    }
    sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

int obter_indice(int x, int y)
{                   // Recebe as coordenadas e converte para o índice da matriz linear
    int novo_y = y; // Mantém a ordem das linhas natural
    if (novo_y % 2 == 0)
    {
        return novo_y * 5 + (4 - x); // Linha par (da direita para a esquerda)
    }
    else
    {
        return novo_y * 5 + x; // Linha ímpar (da esquerda para a direita)
    }
}

void obter_xy(int indice, int *x, int *y)
{                         // Recebe o índice e retorna as coordenadas
    indice = 24 - indice; // Reverter o mapeamento

    *y = indice / 5;      // Calcula a linha
    int pos = indice % 5; // Calcula a posição na linha

    // Se a linha for par, a ordem é esquerda → direita
    // Se a linha for ímpar, a ordem é direita → esquerda
    if (*y % 2 == 0)
    {
        *x = pos;
    }
    else
    {
        *x = 4 - pos;
    }
}

void atualizar_buffer_led()
{ // Atualiza o buffer de LEDs
    for (int linha = 0; linha < 5; linha++)
    {
        for (int coluna = 0; coluna < 5; coluna++)
        {
            int posicao = obter_indice(coluna, linha);
            definir_led(posicao, matriz_led[coluna][linha][0], matriz_led[coluna][linha][1], matriz_led[coluna][linha][2]);
        }
    }
}

int mudar_cor_led(int endereco_led_x, int endereco_led_y, int cor, int brilho)
{ // Muda a cor do LED conforme a cor e o brilho
    matriz_led[endereco_led_x][endereco_led_y][cor] = brilho;
    atualizar_buffer_led();
    escrever_leds();
}

int mudar_estado_led(int endereco_led_x, int endereco_led_y, int estado)
{
    // Muda a cor do LED conforme o estado
    if (estado == 0)
    {
        // Estado 0: LED desligado (off)
        matriz_led[endereco_led_x][endereco_led_y][0] = 0; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 0; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 0; // Blue
    }
    else if (estado == 1)
    {
        // Estado 1: Solo seco (vermelho)
        matriz_led[endereco_led_x][endereco_led_y][0] = 2; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 0; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 0; // Blue
    }
    else if (estado == 2)
    {
        // Estado 2: Solo meio seco (magenta)
        matriz_led[endereco_led_x][endereco_led_y][0] = 2; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 0; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 2; // Blue
    }
    else if (estado == 3)
    {
        // Estado 3: Solo mediano (amarelo)
        matriz_led[endereco_led_x][endereco_led_y][0] = 2; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 2; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 0; // Blue
    }
    else if (estado == 4)
    {
        // Estado 4: Solo úmido (ciano)
        matriz_led[endereco_led_x][endereco_led_y][0] = 0; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 2; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 2; // Blue
    }
    else if (estado == 5)
    {
        // Estado 5: Solo muito úmido (verde)
        matriz_led[endereco_led_x][endereco_led_y][0] = 0; // Red
        matriz_led[endereco_led_x][endereco_led_y][1] = 2; // Green
        matriz_led[endereco_led_x][endereco_led_y][2] = 0; // Blue
    }

    // Se a flag de LED estiver ativada
    if (flag_led == true)
    {
        atualizar_buffer_led(); // Atualiza o buffer dos LEDs com os novos estados
        escrever_leds();        // Envia os dados atualizados para os LEDs
    }
}
int selecionar_estado(int led_x, int led_y, int estado_inicial)
{
    // Função para piscar o LED
    // Se a flag de LED estiver desativada, retorna o estado inicial sem alterar nada
    if (flag_led == false)
    {
        return estado_inicial;
    }

    // Se o estado inicial não for 0 (LED desligado)
    if (estado_inicial != 0)
    {
        // Alterna o estado de piscar
        if (piscar == 1)
        {
            // Se piscar está ativo, desliga o LED
            mudar_estado_led(led_x, led_y, 0);
            piscar = 0; // Atualiza a variável de controle para o próximo piscar
        }
        else
        {
            // Se piscar está inativo, liga o LED com o estado inicial
            mudar_estado_led(led_x, led_y, estado_inicial);
            piscar = 1; // Atualiza a variável de controle para o próximo piscar
        }
        return estado_inicial; // Retorna o estado inicial
    }
    else
    {
        // Se o estado inicial for 0 (LED desligado), alterna entre duas cores
        if (piscar == 1)
        {
            // Se piscar está ativo, muda a cor do LED para magenta (2)
            mudar_cor_led(led_x, led_y, 2, 1);
            piscar = 0; // Atualiza a variável de controle para o próximo piscar
        }
        else
        {
            // Se piscar está inativo, muda a cor do LED para desligado (0)
            mudar_cor_led(led_x, led_y, 2, 0);
            piscar = 1; // Atualiza a variável de controle para o próximo piscar
        }
        return estado_inicial; // Retorna o estado inicial
    }
}

// Funções para acionar o irrigamento
int matriz_acionar_irrig[5][5] = {0}; // Matriz para acionar o irrigamento
void gerar_string_irrig(char *saida)
{ // Gera uma string com a matriz de acionamento do irrigamento
    char buffer[128] = "";
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            char temp[4];
            snprintf(temp, sizeof(temp), "%d", matriz_acionar_irrig[i][j]);
            strcat(buffer, temp);
            if (j < 4)
                strcat(buffer, "|"); // Coloca "|" entre os valores
        }
        if (i < 4)
            strcat(buffer, "/"); // Adiciona "/" entre as linhas
    }
    strcpy(saida, buffer);
}

// Definições para o servidor NTP
#define SERVIDOR_NTP "pool.ntp.org" // Servidor NTP
#define PORTA_NTP 123               // Porta padrão do NTP
#define TAMANHO_MENSAGEM_NTP 48     // Tamanho da mensagem NTP
#define DELTA_NTP 2208988800UL      // 1970-01-01 00:00:00

// Funções para o NTP
struct udp_pcb *pcb_ntp;             // PCB para comunicação UDP com o servidor NTP
ip_addr_t endereco_servidor_ntp;     // Endereço IP do servidor NTP
char string_hora_ntp[30];            // Variável global para armazenar a hora formatada
int offset_fuso_horario = -3 * 3600; // Ajuste para UTC-3 (Brasília)

void callback_recebimento_ntp(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t porta)
{
    // Callback para receber a resposta NTP
    // Verifica se o pacote recebido é válido e se tem o tamanho correto
    if (p && p->tot_len == TAMANHO_MENSAGEM_NTP)
    {
        uint8_t dados_ntp[4]; // Array para armazenar os dados NTP
        // Copia os 4 bytes relevantes da mensagem NTP a partir do offset 40
        pbuf_copy_partial(p, dados_ntp, 4, 40);
        // Converte os dados NTP para um valor de segundos desde 1970 e ajusta pelo delta NTP
        uint32_t segundos_desde_1970 = (dados_ntp[0] << 24 | dados_ntp[1] << 16 | dados_ntp[2] << 8 | dados_ntp[3]) - DELTA_NTP;

        // Adiciona o offset de fuso horário ao tempo bruto
        time_t tempo_cru = segundos_desde_1970 + offset_fuso_horario;
        // Converte o tempo bruto para a estrutura tm usando gmtime (UTC)
        struct tm *info_tempo = gmtime(&tempo_cru);

        // Formata a string com a hora NTP em um formato legível
        snprintf(string_hora_ntp, sizeof(string_hora_ntp), "%02d/%02d/%04d %02d:%02d:%02d UTC",
                 info_tempo->tm_mday, info_tempo->tm_mon + 1, info_tempo->tm_year + 1900,
                 info_tempo->tm_hour, info_tempo->tm_min, info_tempo->tm_sec);
    }

    // Libera o buffer do pacote recebido
    pbuf_free(p);
    // Remove o PCB após processar a resposta
    udp_remove(pcb);
}

void requisicao_ntp()
{
    // Envia uma requisição NTP para o servidor
    // Aloca um buffer para a mensagem NTP
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, TAMANHO_MENSAGEM_NTP, PBUF_RAM);
    if (!p)
        return; // Se a alocação falhar, sai da função

    uint8_t *req = (uint8_t *)p->payload; // Obtém o ponteiro para o payload do buffer
    memset(req, 0, TAMANHO_MENSAGEM_NTP); // Zera a mensagem
    req[0] = 0x1B;                        // Define o primeiro byte da mensagem NTP (configuração padrão)

    // Envia a requisição NTP para o servidor especificado
    udp_sendto(pcb_ntp, p, &endereco_servidor_ntp, PORTA_NTP);
    // Libera o buffer após o envio
    pbuf_free(p);
}

void callback_dns_ntp(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    // Callback para resolver o endereço do servidor NTP
    if (ipaddr)
    {
        // Se o endereço IP foi resolvido com sucesso, armazena-o
        endereco_servidor_ntp = *ipaddr;
        // Envia a requisição NTP após a resolução do DNS
        requisicao_ntp();
    }
    else
    {
        // Se a resolução DNS falhar, imprime uma mensagem de erro
        printf("Falha na resolução DNS\n");
    }
}

const char *iniciar_ntp()
{ // Inicia o processo de obtenção da hora via NTP
    if (pcb_ntp)
    {
        udp_remove(pcb_ntp); // Libera o UDP PCB anterior antes de criar um novo
    }

    pcb_ntp = udp_new();
    if (!pcb_ntp)
    {
        printf("Falha ao criar UDP PCB\n");
        return NULL;
    }
    udp_recv(pcb_ntp, callback_recebimento_ntp, NULL);

    // Configura o DNS manualmente
    ip_addr_t servidor_dns;
    IP4_ADDR(&servidor_dns, 8, 8, 8, 8);
    dns_setserver(0, &servidor_dns);

    // Tenta resolver o endereço do servidor NTP
    err_t err = dns_gethostbyname(SERVIDOR_NTP, &endereco_servidor_ntp, callback_dns_ntp, NULL);
    if (err == ERR_OK)
    {
        requisicao_ntp(); // Se já tivermos um IP, enviamos a requisição NTP
    }
    else if (err == ERR_INPROGRESS)
    {
        printf("Aguardando resposta do DNS...\n");
    }
    else
    {
        printf("Erro na busca DNS!\n");
        return NULL;
    }

    return string_hora_ntp; // Retorna a string formatada com a hora
}

char horarios[5][5][30] = {""}; // Matriz para guardar o horário de atualização de cada ponto

// Funções para o joystick
int pos_x = 2; // Posição inicial do seletor no eixo x
int pos_y = 2; // Posição inicial do seletor no eixo y
bool ultimo_botao1 = false;
bool ultimo_botao2 = false;
uint32_t ultima_atualizacao = 0;

void mover_esquerda()
{
    // Função para mover a posição para a esquerda
    printf("Movimento detectado: ESQUERDA\n");
    if (pos_x > 0) // Verifica se a posição x não é menor que 0
    {
        pos_x--; // Decrementa a posição x
    }
    else
    {
        pos_x = 0; // Garante que a posição x não fique abaixo de 0
    }
    printf("%d", pos_x); // Exibe a nova posição x
}

void mover_direita()
{
    // Função para mover a posição para a direita
    printf("Movimento detectado: DIREITA\n");
    if (pos_x < 4) // Verifica se a posição x não ultrapassa 4
    {
        pos_x++; // Incrementa a posição x
    }
    else
    {
        pos_x = 4; // Garante que a posição x não ultrapasse 4
    }
    printf("%d", pos_x); // Exibe a nova posição x
}

void mover_cima()
{
    // Função para mover a posição para cima
    printf("Movimento detectado: CIMA\n");
    if (pos_y < 4) // Verifica se a posição y não ultrapassa 4
    {
        pos_y++; // Incrementa a posição y
    }
    else
    {
        pos_y = 4; // Garante que a posição y não ultrapasse 4
    }
    printf("%d", pos_y); // Exibe a nova posição y
}

void mover_baixo()
{
    // Função para mover a posição para baixo
    printf("Movimento detectado: BAIXO\n");
    if (pos_y > 0) // Verifica se a posição y não é menor que 0
    {
        pos_y--; // Decrementa a posição y
    }
    else
    {
        pos_y = 0; // Garante que a posição y não fique abaixo de 0
    }
    printf("%d", pos_y); // Exibe a nova posição y
}

void mover_joystick(int x, int y)
{
    // Função para mover a posição com base nas entradas do joystick
    if (flag_led)
    {
        // Atualiza o estado do LED para garantir que ele retorne ao estado inicial ao mover
        mudar_estado_led(pos_x, pos_y, matriz_estado[pos_x][pos_y]);
    }

    // Verifica a posição x do joystick e move para baixo se estiver à esquerda do centro
    if (x < CENTRO_X - LIMIAR)
    {
        mover_baixo();
    }
    // Verifica a posição x do joystick e move para cima se estiver à direita do centro
    else if (x > CENTRO_X + LIMIAR)
    {
        mover_cima();
    }
    // Verifica a posição y do joystick e move para a esquerda se estiver acima do centro
    if (y < CENTRO_Y - LIMIAR)
    {
        mover_esquerda();
    }
    // Verifica a posição y do joystick e move para a direita se estiver abaixo do centro
    else if (y > CENTRO_Y + LIMIAR)
    {
        mover_direita();
    }
}

void pressionar_botao()
{
    // Função chamada quando o botão do joystick é pressionado
    printf("Botão do joystick pressionado!\n");
    flag_led = !flag_led; // Alterna o estado da flag do LED
    if (flag_led == true)
    {
        exibir("Irrig | ON! ", 6); // Exibe mensagem de ativação
        // Atualiza o estado de todos os LEDs conforme a matriz_estado
        for (int x = 0; x < 5; x++)
        {
            for (int y = 0; y < 5; y++)
            {
                mudar_estado_led(x, y, matriz_estado[x][y]); // Muda o estado dos LEDs
            }
        }
        atualizar_buffer_led(); // Atualiza o buffer dos LEDs
        escrever_leds();        // Escreve os estados dos LEDs
    }
    else
    {
        exibir("Irrig | OFF!", 6); // Exibe mensagem de desativação
        limpar_leds();             // Limpa o estado dos LEDs
        escrever_leds();           // Escreve o estado dos LEDs
        gpio_put(LED_VERDE, 0);    // Desliga o LED verde
        gpio_put(LED_VERMELHO, 0); // Desliga o LED vermelho
    }
}

void pressionar_botao_A()
{
    // Função chamada quando o botão A é pressionado
    printf("Botão A pressionado!\n");
    if (flag_selecao == true) // Verifica se a seleção está ativa
    {
        // Alterna o estado na matriz de acionamento de irrigação
        matriz_acionar_irrig[pos_x][pos_y] = !matriz_acionar_irrig[pos_x][pos_y];
    }
}

void pressionar_botao_B()
{
    // Função chamada quando o botão B é pressionado
    printf("Botão B pressionado!\n");
    flag_selecao = !flag_selecao; // Alterna a flag de seleção
    printf("Fim função B\n");     // Mensagem de fim da função
}

void callback_botao(uint gpio, uint32_t eventos)
{
    // Callback para lidar com eventos de botão
    if (gpio == PINO_SW) // Verifica se o botão pressionado é o switch
    {
        pressionar_botao(); // Chama a função para pressionar o botão
    }
    else if (gpio == PINO_BOTAO_A) // Verifica se o botão pressionado é o botão A
    {
        pressionar_botao_A(); // Chama a função para pressionar o botão A
    }
    else if (gpio == PINO_BOTAO_B) // Verifica se o botão pressionado é o botão B
    {
        pressionar_botao_B(); // Chama a função para pressionar o botão B
    }
}

// Funções para o display OLED
uint8_t ssd[ssd1306_buffer_length]; // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
struct render_area area_frame = {   // Área de renderização
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

void exibir(char *texto, int linha)
{                                          // Escrever uma linha
    int y = linha * 8;                     // Calcula a posição vertical com base na linha desejada
    ssd1306_draw_string(ssd, 0, y, texto); // Passa o ponteiro para a string
    render_on_display(ssd, &area_frame);
}

void exibir_multilinha(char *textos[], int tamanho)
{ // Escrever várias linhas de uma vez
    int y = 0;
    for (int i = 0; i < tamanho; i++)
    {
        ssd1306_draw_string(ssd, 0, y, textos[i]);
        y += 8;
    }
    render_on_display(ssd, &area_frame);
}

void limpar_display(uint8_t *ssd, int linha)
{ // Limpar linha
    // Se linha for 0, limpa o display inteiro
    if (linha == 0)
    {
        memset(ssd, 0, 128 * 8); // Limpa o display inteiro (assumindo que a largura é 128 e a altura é 8)
    }
    else
    {
        int y = linha * 8; // Calcula a posição da linha
        // Limpa a linha específica (assumindo que o display tem 128 pixels de largura)
        memset(ssd + (y * 128), 0, 128); // Limpa a linha no display
    }

    render_on_display(ssd, &area_frame); // Atualiza o display
}

void exibir_horario(int x, int y)
{ // Mostra o horário diretamente no display
    exibir(" ", 4);
    if (strlen(horarios[x][y]) == 0)
    {
        // Exibe no OLED
        exibir("Acione | A |", 5);
    }
    else
    {
        char linha1[16] = {0};
        char linha2[16] = {0};
        // Pegando o horário salvo na matriz
        char *horario = horarios[x][y];
        // Separa em duas partes
        strncpy(linha1, horario + 11, 8); // Hora (Ex: "14:30:00")
        strncpy(linha2, horario, 5);      // Data (Ex: "02/02")
        // Formata a string com o caractere "→" antes da hora
        char linha_formatada[16] = {0};
        snprintf(linha_formatada, sizeof(linha_formatada), "%s-%s", linha1, linha2);

        printf("%s\n", linha_formatada); // Corrigido para evitar comportamento indefinido
        exibir(linha_formatada, 5);
    }
}
const char *PAGINA_HTML = ""
                          "<html><head><title>Irrig Smart</title>"
                          "<style>"
                          "    body { "
                          "        background-color: #ADD8E6; /* Azul claro */"
                          "        text-align: center; "
                          "        font-family: Arial, sans-serif; "
                          "    } "
                          "    h1 { "
                          "        color: #000080; /* Azul escuro para o título */"
                          "    } "
                          "    table { "
                          "        margin: 0 auto; "
                          "        border-collapse: collapse; "
                          "    } "
                          "    th, td { "
                          "        padding: 10px; "
                          "        border: 1px solid #000; "
                          "    } "
                          "    th { "
                          "        background-color: #87CEEB; /* Azul claro para o cabeçalho */"
                          "    } "
                          "    .plantas { "
                          "        margin-top: 20px; "
                          "        font-size: 18px; "
                          "        color: #000080; "
                          "    } "
                          "</style>"
                          "</head><body>"
                          "<h1>Monitoramento dos Irrigadores</h1>"
                          "<p>(0 = DESLIGADO, 1 = LIGADO)</p>"
                          "<table>"
                          "<script>"
                          "function toggleIrrig(x, y) {"
                          "    let cell = document.querySelector(`td[data-x='${x}'][data-y='${y}']`);"
                          "    let currentValue = parseInt(cell.innerText);"
                          "    let newValue = currentValue === 1 ? 0 : 1;"
                          "    fetch(`/irrig/${x}/${y}/${newValue}`)"
                          "        .then(response => {"
                          "             location.reload();"
                          "        })"
                          "        .catch(error => console.error('Erro na requisição:', error));"
                          "}"
                          "</script>"
                          "<tbody>%s</tbody></table>"
                          "<div class='plantas'>"
                          "    <h2>Tipos de Plantas</h2>"
                          "    <ul>"
                          "        <li>1. Rosas</li>"
                          "        <li>2. Arbustos</li>"
                          "        <li>3. Fruteiras</li>"
                          "        <li>4. Medicinais</li>"
                          "        <li>5. Cactaceas</li>"
                          "    </ul>"
                          "</div>"
                          "</body></html>";
bool servWeb_theangSpeak(const char *requisicao)
{ // Verifica se a requisição veio da Raspberry PI
    return strstr(requisicao, "User-Agent: ESP") != NULL;
}

void gerar_pagina_html(char *buffer)
{
    // Função para gerar uma tabela HTML a partir da matriz de acionamento
    char temp[256];
    buffer[0] = '\0';            // Inicializa o buffer
    strcat(buffer, "<tr>");      // Inicia a linha da tabela
    for (int i = 4; i >= 0; i--) // Itera sobre as linhas da matriz (de cima para baixo)
    {
        for (int j = 0; j < 5; j++) // Itera sobre as colunas da matriz (da esquerda para a direita)
        {
            // Gera uma célula da tabela com dados da matriz de acionamento
            sprintf(temp, "<td data-x=\"%d\" data-y=\"%d\" onclick=\"toggleIrrig(%d,%d)\">%d</td>",
                    j, i, j, i, matriz_acionar_irrig[j][i]);
            strcat(buffer, temp); // Adiciona a célula ao buffer
        }
        strcat(buffer, "</tr>"); // Fecha a linha da tabela
    }
}

void gerar_string_status(char *buffer)
{
    // Gera uma string representando a matriz de status para retorno ao ESP
    char temp[64];
    buffer[0] = '\0';           // Inicializa o buffer
    for (int i = 0; i < 5; i++) // Itera sobre as linhas da matriz
    {
        for (int j = 0; j < 5; j++) // Itera sobre as colunas da matriz
        {
            sprintf(temp, "%d|", matriz_estado[i][j]); // Formata o valor da matriz
            strcat(buffer, temp);                      // Adiciona o valor ao buffer
        }
        strcat(buffer, "/"); // Adiciona um delimitador entre as linhas
    }
}

static err_t callback_http(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    // Callback para lidar com requisições HTTP
    if (p == NULL) // Verifica se o pacote é nulo
    {
        tcp_close(tpcb); // Fecha a conexão TCP se o pacote for nulo
        return ERR_OK;   // Retorna erro zero
    }
    char *requisicao = (char *)p->payload;  // Obtém o payload da requisição
    processar_requisicao(requisicao, tpcb); // Processa a requisição
    pbuf_free(p);                           // Libera o buffer do pacote
    return ERR_OK;                          // Retorna erro zero
}

void processar_requisicao(char *req, struct tcp_pcb *tpcb)
{
    // Processa a requisição HTTP
    int pos_x, pos_y, estado;            // Variáveis para armazenar coordenadas e estado
    bool esp = servWeb_theangSpeak(req); // Verifica se a requisição é para o serviço web
    char *start = strstr(req, "GET ");   // Procura pela linha de requisição GET
    if (start)
    {
        start += 4;                     // Move o ponteiro para o início do caminho da requisição
        char *end = strchr(start, ' '); // Encontra o final do caminho
        if (end)
        {
            *end = '\0'; // Termina a string no final do caminho
        }
        printf("Requisição recebida: %s\n", start); // Exibe a requisição recebida
    }

    // Verifica se a requisição é para obter o status
    if (strcmp(start, "/status") == 0)
    {
        if (esp) // Se a requisição for para o serviço web
        {
            char status_str[512];
            gerar_string_irrig(status_str); // Gera a string de status
            char resposta[1024];
            // Monta a resposta HTTP com o status
            snprintf(resposta, sizeof(resposta),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n\r\n"
                     "%s",
                     (int)strlen(status_str), status_str);
            tcp_write(tpcb, resposta, strlen(resposta), TCP_WRITE_FLAG_COPY); // Envia a resposta
        }
        else // Se não for para o serviço web
        {
            char buffer_html[2048];
            gerar_pagina_html(buffer_html); // Gera a página HTML
            char buffer[4096];
            // Monta a página HTML completa com a tabela dinâmica
            snprintf(buffer, sizeof(buffer),
                     PAGINA_HTML,
                     buffer_html);
            tcp_write(tpcb, (const char *)buffer, strlen(buffer), TCP_WRITE_FLAG_COPY); // Envia a página HTML
        }
    }
    // Verifica se a requisição contém coordenadas e estado
    else if (sscanf(start, "/%d/%d/%d", &pos_x, &pos_y, &estado) == 3)
    {
        // Atualiza a matriz com os valores recebidos
        if (pos_x >= 0 && pos_x < 5 && pos_y >= 0 && pos_y < 5 && estado >= 0 && estado < 6)
        {
            matriz_estado[pos_x][pos_y] = estado; // Atualiza o estado na matriz

            const char *horario_atual = iniciar_ntp();                                      // Obtém o horário atual
            strncpy(horarios[pos_x][pos_y], horario_atual, sizeof(horarios[pos_x][pos_y])); // Armazena o horário
            mudar_estado_led(pos_x, pos_y, estado);                                         // Atualiza o estado do LED
            printf("%s\n", horarios[pos_x][pos_y]);                                         // Exibe o horário armazenado
            printf("Pos [%d][%d] atualizado para %d (Horário: %s)\n",
                   pos_x, pos_y, estado, horarios[pos_x][pos_y]);

            // Envia resposta de sucesso
            char resposta[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nLED atualizado!";
            tcp_write(tpcb, resposta, strlen(resposta), TCP_WRITE_FLAG_COPY);
        }
        else
        {
            // Envia resposta de erro para coordenadas ou estado inválido
            char resposta[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nErro: LED ou estado inválido!";
            tcp_write(tpcb, resposta, strlen(resposta), TCP_WRITE_FLAG_COPY);
        }
    }
    // Verifica se a requisição é para alterar a irrigação
    else if (strncmp(start, "/irrig/", 7) == 0)
    {
        int x, y, estado;
        if (sscanf(start, "/irrig/%d/%d/%d", &x, &y, &estado) == 3)
        {
            if (x >= 0 && x < 5 && y >= 0 && y < 5)
            {
                matriz_acionar_irrig[x][y] = estado; // Atualiza a matriz de acionamento
                printf("Irrigação em [%d][%d] alterada para %d\n", x, y, estado);
            }
        }
        // Após alterar, gere a nova página HTML
        char buffer_html[2048];
        gerar_pagina_html(buffer_html); // Gera a nova página HTML
        snprintf((char *)buffer_html, sizeof(buffer_html), PAGINA_HTML, buffer_html);
        tcp_write(tpcb, buffer_html, strlen(buffer_html), TCP_WRITE_FLAG_COPY); // Envia a nova página HTML
    }
    else
    {
        // Se a requisição não corresponder a nenhum caso, gera a página HTML padrão
        char buffer_html[2048];
        gerar_pagina_html(buffer_html); // Gera a página HTML
        char buffer[4096];
        // Monta a página HTML completa com a tabela dinâmica
        snprintf(buffer, sizeof(buffer),
                 PAGINA_HTML,
                 buffer_html);
        tcp_write(tpcb, (const char *)buffer, strlen(buffer), TCP_WRITE_FLAG_COPY); // Envia a página HTML
    }
}

static err_t callback_conexao(void *arg, struct tcp_pcb *novo_pcb, err_t err)
{ // Callback para conexões HTTP
    tcp_recv(novo_pcb, callback_http);
    return ERR_OK;
}

static void iniciar_servidor_http(void)
{
    // Inicialização do servidor HTTP
    struct tcp_pcb *pcb = tcp_new(); // Cria um novo PCB (Protocol Control Block) para o servidor
    if (!pcb)                        // Verifica se a criação do PCB falhou
    {
        printf("Erro ao criar PCB\n");
        return; // Sai da função em caso de erro
    }

    // Tenta vincular o PCB ao endereço IP e porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return; // Sai da função se não conseguir vincular
    }

    pcb = tcp_listen(pcb);                            // Coloca o PCB em modo de escuta
    tcp_accept(pcb, callback_conexao);                // Define a função de callback para novas conexões
    printf("Servidor HTTP rodando na porta 80...\n"); // Mensagem de status
}

time_t tempo_inicial;                // Variável para armazenar o tempo de início do programa
uint32_t ultima_atualizacao_relogio; // Variável para armazenar o tempo da última atualização do relógio

bool wifi_conectado()
{
    // Verifica se a conexão Wi-Fi está ativa
    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}

void verificar_conexao_wifi()
{
    // Verifica a conexão Wi-Fi e tenta reconectar se necessário
    while (!wifi_conectado()) // Enquanto não estiver conectado ao Wi-Fi
    {
        printf("Wi-Fi desconectado...\n"); // Mensagem de desconexão
        limpar_display(ssd, 0);            // Limpa o display
        exibir("Aguarde...", 2);           // Exibe mensagem de espera
        // Tenta reconectar ao Wi-Fi com timeout de 10 segundos
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_SENHA, CYW43_AUTH_WPA2_AES_PSK, 10000) == 0)
        {
            printf("Reconectado ao Wi-Fi.\n"); // Mensagem de reconexão
            exibir("Wi-Fi OK!", 6);            // Exibe mensagem de sucesso
            sleep_ms(1000);                    // Espera 1 segundo
            limpar_display(ssd, 0);            // Limpa o display novamente

            // Atualiza o horário do relógio via NTP
            const char *horario_ntp = iniciar_ntp();
            if (horario_ntp != NULL) // Se o horário NTP foi obtido com sucesso
            {
                struct tm info_tempo;
                // Formata a string do horário NTP para a estrutura tm
                sscanf(horario_ntp, "%d/%d/%d %d:%d/%d", &info_tempo.tm_mday, &info_tempo.tm_mon, &info_tempo.tm_year, &info_tempo.tm_hour, &info_tempo.tm_min, &info_tempo.tm_sec);
                info_tempo.tm_year -= 1900;                // Ajusta o ano para o formato correto
                info_tempo.tm_mon -= 1;                    // Ajusta o mês para o formato correto
                tempo_inicial = mktime(&info_tempo);       // Converte para time_t
                ultima_atualizacao_relogio = time_us_32(); // Atualiza o tempo da última atualização do relógio
            }
            else
            {
                exibir("Erro ao obter hora", 7); // Mensagem de erro ao obter horário
            }
            break; // Sai do loop após reconectar
        }
        sleep_ms(2000); // Espera 2 segundos antes de tentar novamente
    }
}

time_t obter_hora_valida_ntp()
{
    time_t hora_valida = 0;  // Inicializa a variável de hora válida
    while (hora_valida == 0) // Continua tentando até obter uma hora válida
    {
        const char *horario_ntp = iniciar_ntp(); // Tenta obter o horário NTP
        if (horario_ntp != NULL)                 // Se o horário NTP foi obtido com sucesso
        {
            struct tm info_tempo;
            // Formata a string do horário NTP para a estrutura tm
            sscanf(horario_ntp, "%d/%d/%d %d/%d/%d", &info_tempo.tm_mday, &info_tempo.tm_mon, &info_tempo.tm_year, &info_tempo.tm_hour, &info_tempo.tm_min, &info_tempo.tm_sec);
            info_tempo.tm_year -= 1900;        // Ajusta o ano para o formato correto
            info_tempo.tm_mon -= 1;            // Ajusta o mês para o formato correto
            hora_valida = mktime(&info_tempo); // Converte para time_t
            // Reinicia a hora válida se o horário for 00:00:00
            if (info_tempo.tm_hour == 0 && info_tempo.tm_min == 0 && info_tempo.tm_sec == 0)
            {
                hora_valida = 0; // Reinicia se o horário for 00:00:00
            }
        }
        if (hora_valida == 0) // Se a hora válida ainda não foi obtida
        {
            exibir("Obtendo hora válida...", 7); // Mensagem de espera
            sleep_ms(2000);                      // Espera 2 segundos antes de tentar novamente
        }
    }
    return hora_valida; // Retorna a hora válida obtida
}

// Função principal
int main()
{
    stdio_init_all();                                                                            // Inicializa a saída padrão
    adc_init();                                                                                  // Inicializa o ADC
    inicializar_leds(PINO_LED);                                                                  // Inicializa a matriz de LEDs
    gpio_init(LED_VERMELHO);                                                                     // Inicializa o pino do LED vermelho
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);                                                        // Configura o pino do LED vermelho como saída
    gpio_init(LED_VERDE);                                                                        // Inicializa o pino do LED verde
    gpio_set_dir(LED_VERDE, GPIO_OUT);                                                           // Configura o pino do LED verde como saída
    gpio_set_dir(LED_AZUL, GPIO_OUT);                                                            // Configura o pino do LED azul como saída
    adc_gpio_init(PINO_VRY);                                                                     // Inicializa o pino do eixo Y do joystick
    adc_gpio_init(PINO_VRX);                                                                     // Inicializa o pino do eixo X do joystick
    gpio_init(PINO_SW);                                                                          // Inicializa o pino do botão do joystick
    gpio_set_dir(PINO_SW, GPIO_IN);                                                              // Configura o pino do botão como entrada
    gpio_pull_up(PINO_SW);                                                                       // Habilita o pull-up interno
    gpio_init(PINO_BOTAO_A);                                                                     // Inicializa o pino do botão A
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);                                                         // Configura o pino do botão como entrada
    gpio_pull_up(PINO_BOTAO_A);                                                                  // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &callback_botao); // Habilita a interrupção no botão A
    gpio_init(PINO_BOTAO_B);                                                                     // Inicializa o pino do botão B
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);                                                         // Configura o pino do botão como entrada
    gpio_pull_up(PINO_BOTAO_B);                                                                  // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &callback_botao); // Habilita a interrupção no botão B
    int ultimo_x = CENTRO_X, ultimo_y = CENTRO_Y;                                                // Última posição do joystick
    bool ultimo_sw = true;                                                                       // Último estado do botão
    // Inicialização do I2C
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);  // Inicializa o I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura os pinos do I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura os pinos do I2C
    gpio_pull_up(I2C_SDA);                     // Habilita o pull-up nos pinos do I2C
    gpio_pull_up(I2C_SCL);                     // Habilita o pull-up nos pinos do I2C
    // Processo de inicialização do OLED SSD1306
    ssd1306_init();
    calculate_render_area_buffer_length(&area_frame);
    memset(ssd, 0, ssd1306_buffer_length); // Limpa o buffer do display
    render_on_display(ssd, &area_frame);   // Atualiza o display
    exibir("Ligando...", 1);
reiniciar: // Reinicia o programa
    sleep_ms(2000);
    // Inicializa o Wi-Fi
    if (cyw43_arch_init())
    {
        return 1;
    }
    cyw43_arch_enable_sta_mode(); // Habilita o modo estação (cliente)
    limpar_display(ssd, 0);
    exibir("Conectando Wifi...", 1);
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_SENHA, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0)
    { // Tenta conectar ao Wi-Fi
        printf("Falha na conexão Wi-Fi, tentando novamente...\n");
        exibir("tentando...", 2);
        sleep_ms(100);
    }
    // Exibe o endereço IP
    uint8_t *endereco_ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char string_ip[16]; // Espaço suficiente para "255.255.255.255\0"
    sprintf(string_ip, "%d.%d.%d.%d", endereco_ip[0], endereco_ip[1], endereco_ip[2], endereco_ip[3]);
    exibir("Wi-Fi OK! IP:", 2);
    exibir(string_ip, 3);
    sleep_ms(3000);
    limpar_display(ssd, 0);
    // Inicia o servidor HTTP
    iniciar_servidor_http();
    iniciar_ntp(); // Inicia o NTP
    sleep_ms(1000);
    limpar_display(ssd, 0);

    // Obter a hora atual do NTP uma vez
    const char *horario_ntp = iniciar_ntp();
    time_t tempo_inicial;
    if (horario_ntp != NULL)
    {
        struct tm info_tempo;
        sscanf(horario_ntp, "%d/%d/%d %d:%d:%d", &info_tempo.tm_mday, &info_tempo.tm_mon, &info_tempo.tm_year, &info_tempo.tm_hour, &info_tempo.tm_min, &info_tempo.tm_sec);
        info_tempo.tm_year -= 1900; // Ajusta o ano
        info_tempo.tm_mon -= 1;     // Ajusta o mês
        tempo_inicial = mktime(&info_tempo);
    }
    else
    {

        return 1;
    }

    uint32_t ultima_atualizacao_relogio = time_us_32();  // Tempo da última atualização do relógio em microssegundos
    uint32_t ultima_atualizacao_joystick = time_us_32(); // Tempo da última atualização do joystick em microssegundos
    bool wifi_conectado = true;                          // Estado da conexão Wi-Fi

    while (true)
    { // Loop principal
        if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP)
        { // Verifica se a conexão Wi-Fi está ativa
            if (wifi_conectado)
            {
                wifi_conectado = false;
                verificar_conexao_wifi(); // Verifica e reconecta ao Wi-Fi se necessário
                wifi_conectado = true;
            }
        }
        uint32_t tempo_atual = time_us_32();    // Pega o tempo atual em microssegundos
        bool valor_sw = gpio_get(PINO_SW) == 0; // Lê o valor do botão
        // Incrementa o tempo
        time_t tempo_atual_seg = tempo_inicial + (tempo_atual - ultima_atualizacao_relogio) / 1000000;
        // Formata a hora para exibir no display
        struct tm *info_tempo = localtime(&tempo_atual_seg);
        char texto_display[32];
        snprintf(texto_display, sizeof(texto_display), "Hora: %02d:%02d:%02d", info_tempo->tm_hour, info_tempo->tm_min, info_tempo->tm_sec);
        exibir(texto_display, 7);
        if (flag_selecao == false)
        { // Se estiver com o cursor desabilitado
            exibir("Irrig Smart    ", 1);
            exibir("___________", 2);
            exibir("Pressione ", 3);
            exibir(" ", 4);
            exibir(" | B | ", 5);
            gpio_put(LED_VERDE, 0);
            gpio_put(LED_VERMELHO, 0);
            mudar_estado_led(pos_x, pos_y, matriz_estado[pos_x][pos_y]); // Garantir que o LED retorne ao estado inicial caso esteja piscando ao pressionar o botão B
        }
        else
        {
            if (flag_led)
            {
                selecionar_estado(pos_x, pos_y, matriz_estado[pos_x][pos_y]); // Coloca a posição do cursor piscando
            }
            exibir("X | Y | Irrigadores", 1);
            exibir_horario(pos_x, pos_y); // Exibe o horário do ponto selecionado
            if (matriz_acionar_irrig[pos_x][pos_y] == 1)
            { // Aciona o irrigador
                exibir("Irrig | ON     ", 3);
                if (flag_led)
                {
                    gpio_put(LED_VERDE, 1);
                    gpio_put(LED_VERMELHO, 0);
                }
            }
            else
            {
                exibir("Irrig | OFF     ", 3);
                if (flag_led)
                {
                    gpio_put(LED_VERDE, 0);
                    gpio_put(LED_VERMELHO, 1);
                }
            }
            char buffer[16]; // Buffer para exibir as coordenadas e o estado do ponto
            char estado[10]; // Buffer para exibir o estado do ponto
            if (matriz_estado[pos_x][pos_y] == 1)
            { // Converte o estado do ponto para string
                strcpy(estado, "Seco    ");
            }
            else if (matriz_estado[pos_x][pos_y] == 2)
            {
                strcpy(estado, "Baixo   ");
            }
            else if (matriz_estado[pos_x][pos_y] == 3)
            {
                strcpy(estado, "Medio   ");
            }
            else if (matriz_estado[pos_x][pos_y] == 4)
            {
                strcpy(estado, "Bom     ");
            }
            else if (matriz_estado[pos_x][pos_y] == 5)
            {
                strcpy(estado, "Alto    ");
            }
            else if (matriz_estado[pos_x][pos_y] == 0)
            {
                strcpy(estado, "Offline ");
            }
            sprintf(buffer, "%d | %d |%s", pos_x, pos_y, estado); // Converte as coordenadas e o estado do ponto para string
            exibir(buffer, 2);

            if (tempo_atual - ultima_atualizacao_joystick >= INTERVALO_ATUALIZACAO)
            { // Verifica se é hora de atualizar o cursor
                // Lê o eixo X
                adc_select_input(0);
                int valor_vrx = adc_read();

                // Lê o eixo Y
                adc_select_input(1);
                int valor_vry = adc_read();

                // Verifica se o joystick se moveu além da zona morta
                if (abs(valor_vrx - CENTRO_X) > ZONA_MORTA || abs(valor_vry - CENTRO_Y) > ZONA_MORTA)
                {
                    if (abs(valor_vrx - ultimo_x) > ZONA_MORTA || abs(valor_vry - ultimo_y) > ZONA_MORTA)
                    {
                        mover_joystick(valor_vrx, valor_vry);
                        ultimo_x = valor_vrx; // Atualiza a última posição do eixo X
                        ultimo_y = valor_vry; // Atualiza a última posição do eixo Y
                    }
                }
                ultima_atualizacao_joystick = tempo_atual; // Atualiza o tempo da última atualização do joystick
            }
        }
        if (valor_sw && !ultimo_sw)
        { // Verifica se o botão foi pressionado
            pressionar_botao();
        }
        ultimo_sw = valor_sw; // Atualiza o valor do botão
        cyw43_arch_poll();    // Necessário para manter o Wi-Fi ativo
        sleep_ms(50);
    }

    cyw43_arch_deinit(); // Desliga o Wi-Fi (não será chamado, pois o loop é infinito)
    return 0;
}
