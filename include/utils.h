#ifndef UTILS
#define UTILS

#include <unistd.h>
#include "protocol.h"

// Cria e retorna mensagem seguindo o protocolo criado
struct protocolo build_message (uint8_t *endDestino, uint8_t *endOrigem, unsigned int sequencia, unsigned int tipo, void *dados, uint16_t tamanho_dados);

// Calcula paridade vertical da mensagem
uint32_t calculo_paridade (struct protocolo *mensagem);

#endif