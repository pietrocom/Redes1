#ifndef UTILS
#define UTILS

#include <unistd.h>
#include "protocol.h"

// Calcula paridade vertical da mensagem
uint32_t calculo_paridade (struct protocolo *mensagem, int tamanho);

// Retorna 1 se a mensagem estiver correta e 0 caso contrario
int verifica_paridade (struct protocolo *mensagem, int tamanho);

// Cria e retorna mensagem seguindo o protocolo criado
struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados);

long long timestamp();

#endif