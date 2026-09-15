/*
Esse arquivo cuida da definicao e correta separacao dos bytes
que serao enviados. 
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

struct protocolo {
    char preambulo [7];
    char marcador_inicio [1];
    char end_destino [6];
    char end_origem [6];
    char tamanho [2];
    char seq_tipo [1];
    char * dados;       // De 45 a 117 bytes
    char crc [4];
};

#endif