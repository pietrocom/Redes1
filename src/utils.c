#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "protocol.h"

uint32_t calculo_paridade (struct protocolo *mensagem, int tamanho) {

    // Em quantos blocos de 4B a mensagem pode ser dividida
    int blocos = tamanho / 4;
    if (tamanho % 4 != 0) {
        blocos++;
    }
    
    uint32_t *buffer = (uint32_t *) mensagem;

    // XOR para calcular se um bit 1 deve ou nao ser adicionado para ter 1's pares
    uint32_t paridade = 0;
    for (int i = 0; i < blocos; i++) {
        paridade = paridade ^ buffer[i];
    }

    return paridade;
}

int verifica_paridade (struct protocolo *mensagem, int tamanho) {

    int blocos = tamanho / 4;
    uint32_t *buffer = (uint32_t *) mensagem;

    // XOR para verificar se o campo CRC adicionado ao resto da mensagem tem 1's pares
    uint32_t paridade = 0;
    for (int i = 0; i < blocos; i++) {
        paridade = paridade ^ buffer[i];
    }

    if (paridade) return 0;
    else return 1;
}

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}
