#include <unistd.h>

#include "protocol.h"

struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados) {

    struct protocolo mensagem;
    memset(&mensagem, 0, sizeof(struct protocolo));

    for (int i = 0; i < 6; i++) {
        mensagem.end_destino[i] = endDestino[i];
        mensagem.end_origem[i] = endOrigem[i];
    }

    mensagem.tamanho = tamanho_dados;

    sequencia = (sequencia << 4) + tipo;
    mensagem.seq_tipo = sequencia;

    if(dados != NULL)    
        memcpy(mensagem.dados, dados, tamanho_dados);

    mensagem.crc = calculo_paridade(&mensagem);

    return mensagem;
}

uint32_t calculo_paridade (struct protocolo *mensagem) {

    // Em quantos blocos de 4B a mensagem pode ser dividida
    int blocos = (sizeof(struct protocolo) - 4) / 4;
    uint32_t *buffer = (uint32_t *) mensagem;

    uint32_t paridade = 0;
    for (int i = 0; i < blocos; i++) {
        paridade = paridade ^ buffer[i];
    }

    return paridade;
}
