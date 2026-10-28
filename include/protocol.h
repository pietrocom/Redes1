/*
Esse arquivo cuida da definicao e correta separacao dos bytes
que serao enviados. 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define TAM_MINIMO 64
#define TAM_MAXIMO 128

#define TIMEOUT 300     // Em milisegundos

#define MACSERVIDOR {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}
#define MACCLIENTE  {0x02, 0x00, 0x00, 0x00, 0x00, 0x02}

#define TIPO_ACK     0x00
#define TIPO_NACK    0x01
#define TIPO_ERRO    0x02
#define TIPO_CD      0x03
#define TIPO_LS      0x04
#define TIPO_GET     0x05
#define TIPO_PUT     0x06
#define TIPO_TEST    0x07
#define TIPO_OK      0x08
#define TIPO_FIM     0x09
#define TIPO_DADOS   0x0d
#define TIPO_TAMANHO 0x0e
#define TIPO_MOSTRAR 0x0f

struct protocolo {
    // O preambulo e o marcador de inicio sao adicionados pela propria interface de rede na camada fisica ao utilizarmos Raw Sockets (AF_PACKET) no Linux
    uint8_t end_destino [6];
    uint8_t end_origem [6];
    uint16_t tamanho;
    char seq_tipo;
    uint8_t dados_crc [121];       // De 45B a 117B + 4B de CRC
};

struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados);

void ack (int soquete, uint8_t * mac_dest, uint8_t * mac_orig);

void nack (int soquete, uint8_t * mac_dest, uint8_t * mac_orig);

#endif