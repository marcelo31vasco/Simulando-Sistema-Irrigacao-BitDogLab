#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <stdbool.h>

volatile bool tempo_decorrido = false;  // Variável global que indica se o tempo de espera já passou.

bool timer_callback(struct repeating_timer *t) {  // Função de callback chamada quando o temporizador expira.
    tempo_decorrido = true;  // Define a variável 'tempo_decorrido' como 'true' para sinalizar que o tempo passou.
    return false;  // Retorna 'false' para desativar o temporizador após o tempo.
}

void esperar_ms(int tempo) {  // Função para esperar um determinado tempo em milissegundos.
    tempo_decorrido = false;  // Reseta a variável 'tempo_decorrido' antes de começar o temporizador.
    struct repeating_timer timer;  // Cria uma instância de um temporizador.
    
    // Configura o temporizador para chamar 'timer_callback' após o tempo especificado.
    add_repeating_timer_ms(tempo, timer_callback, NULL, &timer);
    
    // Loop que espera até que o tempo tenha decorrido, verificando a variável 'tempo_decorrido'.
    while (!tempo_decorrido) {
        tight_loop_contents();  // Garante que o microcontrolador não entre em um estado de baixa potência e execute o loop de forma eficiente.
    }
}
