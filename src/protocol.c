#include <string.h>

#include "utils.h"
#include "protocol.h"
#include "raw_sockets.h"

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
    uint32_t paridade = calculo_paridade(&mensagem, 15 + tamanho_dados);

    // 15 = end_or + end_dest + tam + seq_tipo
    int tamanho_total = 15 + tamanho_dados + 4;
    if (tamanho_total < TAM_MINIMO) tamanho_total = TAM_MINIMO;

    // Insere a paridade logo apos os dados
    uint32_t *posicao_crc = (uint32_t *) ((uint8_t *)&mensagem + (tamanho_total - 4));
    *posicao_crc = paridade;

    return mensagem;
}

int eh_pedido (uint8_t tipo) {
    return tipo == TIPO_CD  || tipo == TIPO_LS  || tipo == TIPO_GET ||
           tipo == TIPO_PUT || tipo == TIPO_TEST;
}

uint8_t tipo (uint8_t seq_tipo) {
    return (seq_tipo & 0x0f);
}

uint8_t seq (uint8_t seq_tipo) {
    return ((seq_tipo & 0xf0) >> 4);
}

int ack (int soquete, uint8_t * mac_dest, uint8_t * mac_orig, uint8_t seq) {

    return send_message(soquete, mac_dest, mac_orig, seq, TIPO_ACK, NULL, 0);
}

int nack (int soquete, uint8_t * mac_dest, uint8_t * mac_orig) {

    return send_message(soquete, mac_dest, mac_orig, 0x00, TIPO_NACK, NULL, 0);
}