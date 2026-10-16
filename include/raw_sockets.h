/* 
Header do arquivo raw_sockets.c. 
*/

#ifndef RAW_SOCKETS_H
#define RAW_SOCKETS_H

#include <stdint.h>

#include "protocol.h"
 
int cria_raw_socket (char * nome_interface_rede);

int send_message (int soquete, uint8_t *mac_dest, uint8_t *mac_orig, uint8_t sequencia, uint8_t tipo, void *dados, uint16_t tamanho_dados);

// Retorna o tamanho lido, -1 (corrompido) ou 0 (timeout) 
int receive_message (int soquete, uint8_t *meu_mac, struct protocolo *pacote_retorno);

#endif