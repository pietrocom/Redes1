#include <unistd.h>

#include "protocol.h"

struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados) {

    struct protocolo mensagem;
    memset(&mensagem, 0, sizeof(struct protocolo));

    // Enderecos de origem e destino
    for (int i = 0; i < 6; i++) {
        mensagem.end_destino[i] = endDestino[i];
        mensagem.end_origem[i] = endOrigem[i];
    }

    // Tamanho dos dados, sequencia e tipo
    mensagem.tamanho = tamanho_dados;
    sequencia = (sequencia << 4) + tipo;
    mensagem.seq_tipo = sequencia;

    // Dados
    memcpy(mensagem.dados_crc, dados, tamanho_dados);

    // Paridade
    uint32_t paridade = calculo_paridade(&mensagem);

    int tamanho_total = 15 + tamanho_dados + 4;
    if (tamanho_total < 64) tamanho_total = 64;

    // Insere a paridade logo apos os dados
    uint32_t *posicao_crc = (uint32_t *) ((uint8_t *)&mensagem + (tamanho_total - 4));
    *posicao_crc = paridade;

    return mensagem;
}

uint32_t calculo_paridade (struct protocolo *mensagem) {

    // Em quantos blocos de 4B a mensagem pode ser dividida
    int blocos = (sizeof(struct protocolo) - 4) / 4;
    uint32_t *buffer = (uint32_t *) mensagem;

    // XOR para calcular se um bit deve ou nao ser adicionado para ter 1's pares
    uint32_t paridade = 0;
    for (int i = 0; i < blocos; i++) {
        paridade = paridade ^ buffer[i];
    }

    return paridade;
}

int verifica_paridade (struct protocolo *mensagem) {

    int blocos = (sizeof(struct protocolo) - 4) / 4;
    uint32_t *buffer = (uint32_t *) mensagem;

    // XOR para verificar se o campo CRC adicionado ao resto da mensagem tem 1's pares
    uint32_t paridade = 0;
    for (int i = 0; i < blocos; i++) {
        paridade = paridade ^ buffer[i];
    }

    if (paridade) return 0;
    else return 1;
}
