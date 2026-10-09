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

struct protocolo {
    unsigned char preambulo [7];
    unsigned char marcador_inicio [1];
    uint8_t end_destino [6];
    uint8_t end_origem [6];
    uint16_t tamanho;
    char seq_tipo;
    uint8_t dados_crc [121];       // De 45 a 117 bytes
};

#endif