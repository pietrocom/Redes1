/*
Esse arquivo cuida da definicao e correta separacao dos bytes
que serao enviados. 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define TAM_MINIMO 64
#define TAM_MAXIMO 128

struct protocolo {
    unsigned char preambulo [7];
    unsigned char marcador_inicio [1];
    uint8_t end_destino [6];
    uint8_t end_origem [6];
    uint16_t tamanho;
    char seq_tipo;
    uint8_t dados[109];       // De 45 a 117 bytes
    uint32_t crc;
};

#endif