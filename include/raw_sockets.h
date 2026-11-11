/* 
Header do arquivo raw_sockets.c. 
*/

#ifndef RAW_SOCKETS_H
#define RAW_SOCKETS_H

#include <stdint.h>

#include "protocol.h"

#define TENT_MAX_ENVIOS 10

enum resposta { 
    RESP_ACK, 
    RESP_NACK, 
    RESP_OUTRA, 
    RESP_CORROMPIDA, 
    RESP_TIMEOUT 
};

// Funcao de autoria do professor Eduardo Todt do DINF. Acessivel no site do mesmo.
int cria_raw_socket (char * nome_interface_rede);

// Retorna o proximo numero de sequencia.
uint8_t proxima_seq ();

// Constroi o pacote, calcula seu tamanho e o envia.
// Retorno: 0 em caso de sucesso; -1 c.c.
int send_message (int soquete, uint8_t *mac_dest, uint8_t *mac_orig, uint8_t sequencia, uint8_t tipo, void *dados, uint16_t tamanho_dados);

// Retorna o tamanho lido, 0 (corrompido) ou -1 (timeout).
int receive_message (int soquete, uint8_t *meu_mac, struct protocolo *pacote_retorno);

// Aguarda alguma resposta ou da timeout.
// Retorno: enum resposta.
int await_response (int soquete, uint8_t * meu_mac, struct protocolo * resposta);

// Envia a mesagem e aguarda a resposta, preenchendo *resposta.
// Caso a resposta recebida seja:
//  1- timeout, corrompida ou NACK: reenvia a mensagem.
//  2- o mesmo pedido (sequencias iguais): trata como NACK e reenvia a mensagem.
//  3- outro pedido (sequencias diferentes): trata como se tivesse recebido um ACK do cliente, este que se perdeu.
//  4- ACK, OK ou ERRO com sequencia igual: Eh a resposta correta. Retorna 0 se ACK, 1 se OK/ERRO.
//  5- ACK, OK ou ERRO com sequencia diferente, descarta essa resposta e espera a correta.
// Reenvia até TENT_MAX_ENVIOS vezes. 
// Note que os casos 2 e 3 acontecem somente com o servidor. O cliente nunca espera pedidos do servidor. 
// Retorno: 1 caso o tipo nao seja ACK nem NACK; 0 caso ACK; -1 em caso de erro.
int send_and_wait (int soquete, uint8_t * mac_dest, uint8_t * mac_orig, uint8_t seq, uint8_t tipo, 
                   void * dados, uint16_t n, struct protocolo * resposta);

#endif