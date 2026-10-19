/*
Esse arquivo cuida da definicao e correta separacao dos bytes
que serao enviados. 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define TAM_MINIMO 64
#define TAM_MAXIMO 128

#define MACSERVIDOR {0x60, 0xc7, 0x27, 0x2d, 0x52, 0x12}
#define MACCLIENTE {0x60, 0xc7, 0x27, 0x2d, 0x52, 0x12}

#define TIPO_ACK 0
#define TIPO_NACK 1
#define TIPO_ERRO 2
#define TIPO_CD 3
#define TIPO_LS 4
#define TIPO_GET 5
#define TIPO_PUT 6
#define TIPO_OK 8
#define TIPO_FIM 9
#define TIPO_DADOS 13
#define TIPO_TAMANHO 14
#define TIPO_MOSTRAR 15

struct protocolo {
    // O preambulo e o marcador de inicio sao adicionados pela propria interface de rede na camada fisica ao utilizarmos Raw Sockets (AF_PACKET) no Linux
    uint8_t end_destino [6];
    uint8_t end_origem [6];
    uint16_t tamanho;
    char seq_tipo;
    uint8_t dados_crc [121];       // De 45B a 117B + 4B de CRC
};

struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados);

#endif