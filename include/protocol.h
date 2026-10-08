/*
Esse arquivo cuida da definicao e correta separacao dos bytes
que serao enviados. 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

struct protocolo {
    unsigned char preambulo [7];
    unsigned char marcador_inicio [1];
    unsigned char end_destino [6];
    unsigned char end_origem [6];
    unsigned char tamanho [2];
    unsigned char seq_tipo [1];
    unsigned char * dados;       // De 45 a 117 bytes
    unsigned char crc [4];
};

#endif