/*
Este arquivo implementa as funcoes que manipulam raw sockets, envio e recebimento de mensagens.
*/
 
#include <sys/socket.h>         // socket(), bind(), setsockopt(), AF_*, SOCK_*, SOL_PACKET
#include <arpa/inet.h>          // Conversao de ordem de bytes
#include <net/ethernet.h>       // Constantes Ethernet
#include <linux/if_packet.h>    // API do AF_PACKET (e.g. sockaddr_ll) | exclusiva do linux
#include <net/if.h>             // Interface de rede <-> indice kernel
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <sys/time.h>

#include "protocol.h"
#include "commands.h"
#include "utils.h"
#include "raw_sockets.h"
 
// --- Variaveis Globais ---

static uint8_t seq_cliente = 0;


// --- Funcoes Estaticas ---

// Confere a paridade de um pacote recebido. Valida o campo tamanho, copia
// cabecalho + dados para pacote_retorno (com o restante zerado) e recalcula
// a paridade sobre esses 15 + tamanho bytes, comparando com os 4 bytes que
// vieram logo apos os dados.
// Retorno: recebidos se a paridade confere; 0 se o pacote estiver corrompido.
static int handle_parity (struct protocolo * pacote_recebido, struct protocolo * pacote_retorno, int recebidos, unsigned char * buffer) {
    uint16_t n = pacote_recebido->tamanho;

    // Tamanho impossivel ou maior do que o que chegou
    if (n > 117 || 19 + n > recebidos) return 0;

    // Copia so cabecalho + dados; o resto de pacote_retorno fica zerado,
    // igual ao que o emissor tinha quando calculou a paridade
    // Isso eh importante pois a comparacao deve ser feita com os bytes apos 
    // os dados zerados, como valiam na hora de mandar a mensagem. 
    memset(pacote_retorno, 0, sizeof(struct protocolo));
    memcpy(pacote_retorno, buffer, 15 + n);

    // Paridade que veio no pacote (4 bytes logo apos os dados)
    uint32_t paridade_recebida;
    memcpy(&paridade_recebida, buffer + 15 + n, 4);

    // Recalcula sobre os mesmos 15 + n bytes e compara
    if (calculo_paridade(pacote_retorno, 15 + n) != paridade_recebida) return 0;

    return recebidos;
}

// Copia para quadro a nova mensagem com o preenchimento
// Retorno: o novo tamanho.
static int aplica_enchimento (struct protocolo * pacote, int tam_atual, uint8_t * quadro) {
    uint8_t byte = 0x00;

    // Comeca copiando os valores do cabecalho, que nao devem ser preenchidos com 0xff
    memcpy(quadro, pacote, 12);

    // Percorre toda a mensagem procurando por bytes 0x81 ou 0x88 e 
    // adicionando 0xff apos os mesmos caso encontre
    int novo_tamanho = 12;  // Comeca depois dos enderecos
    for (int i = 12; i < tam_atual; i++) {
        byte = ((uint8_t *)pacote)[i];
        quadro[novo_tamanho] = byte;

        if (byte == 0x81 || byte == 0x88) {
            novo_tamanho++;
            quadro[novo_tamanho] = 0xff;
        }
        
        novo_tamanho++;
    }

    return novo_tamanho;
}

// Copia do pacote ao quadro todos os bytes, exceto os seguintes a um 0x81 ou 0x88.
// Retorno: o tamanho da mensagem resultante (quadro); -1 se faltar o 0xff (pacote corrompido).
static int remove_enchimento (uint8_t * pacote, int tam_atual, uint8_t * quadro) {
    uint8_t byte;

    // Enderecos vieram sem enchimento
    memcpy(quadro, pacote, 12);

    // Percorre a mesagem recebida copiando os bytes de pacote para quadro
    // Quando encontra 0x81 ou 0x88 em pacote, pula o proximo byte
    int tamanho_quadro = 12;
    for (int i = 12; i < tam_atual; i++) {
        byte = pacote[i];
        quadro[tamanho_quadro] = byte;
        if (byte == 0x81 || byte == 0x88) {
            i++;
            if (i >= tam_atual || pacote[i] != 0xff) return -1;
        }
        tamanho_quadro++;
    }

    return tamanho_quadro;
}


// --- Funcoes da API ---

uint8_t proxima_seq () { 
    seq_cliente = (seq_cliente + 1) & 0x0F;     // Reinicia quando passa de 15
    return seq_cliente; 
}

int cria_raw_socket (char * nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }

    const int timeoutMillis = TIMEOUT;
    struct timeval timeout = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
 
    return soquete;
}

int send_message (int soquete, uint8_t *mac_dest, uint8_t *mac_orig, uint8_t sequencia, uint8_t tipo, void *dados, uint16_t tamanho_dados) {

    // Construcao do pacote
    struct protocolo pacote = build_message(mac_dest, mac_orig, sequencia, tipo, dados, tamanho_dados);

    // Realiza o preenchimento e calcula o tamanho do pacote
    uint8_t quadro [2 * (19 + 117)]; 
    memset(quadro, 0, sizeof(quadro));  // Todo o preenchimento ate 64 sai com zeros
    int tam_envio = aplica_enchimento(&pacote, 19 + tamanho_dados, quadro);
    if (tam_envio < TAM_MINIMO) tam_envio = TAM_MINIMO;

    // Envia pacote
    int enviados = send(soquete, quadro, tam_envio, 0);
    
    return enviados;
}

int receive_message (int soquete, uint8_t *meu_mac, struct protocolo *pacote_retorno) {

    uint8_t buffer[2 * (19 + 117)];
    uint8_t mensagem[sizeof(buffer)];

    while(1) {
        // Recebe e guarda mensagem no buffer
        int recebidos = recv(soquete, buffer, sizeof(buffer), 0);

        if (recebidos > 0) {
            // Verifica endereco de destino
            if (memcmp(buffer, meu_mac, 6) == 0) {
                int tamanho = remove_enchimento(buffer, recebidos, mensagem);
                if (tamanho == -1) return 0;               // enchimento invalido = corrompido
                return handle_parity((struct protocolo *) mensagem, pacote_retorno, tamanho, mensagem);
            }
        } 
        // Caso de timeout
        else if (recebidos == -1) {
            return -1;
        }
    }
}

int await_response (int soquete, uint8_t * meu_mac, struct protocolo * resposta) {

    int status = receive_message(soquete, meu_mac, resposta);

    // Timeout
    if (status == -1) return RESP_TIMEOUT;
    // Mensagem corrompida
    if (status ==  0) return RESP_CORROMPIDA;

    // Mascara 00001111
    uint8_t tipo = resposta->seq_tipo & 0x0F;

    if      (tipo == TIPO_ACK)  return RESP_ACK;
    else if (tipo == TIPO_NACK) return RESP_NACK;
    else                        return RESP_OUTRA;
}

int send_and_wait (int soquete, uint8_t * mac_dest, uint8_t * mac_orig, uint8_t seq,
                   uint8_t tipo, void * dados, uint16_t n, struct protocolo * resposta) {

    struct protocolo local;
    if (!resposta) resposta = &local;

    // Tentara enviar a mensagem ate TENT_MAX_ENVIOS vezes
    for (int envio = 1; envio <= TENT_MAX_ENVIOS; envio++) {
        if (send_message(soquete, mac_dest, mac_orig, seq, tipo, dados, n) == -1) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
            return -1;
        }

        enum resposta status;

        while (1) {
            status = await_response(soquete, mac_orig, resposta);

            // Nesses casos, a sequencia nao pode ser analisada
            // Sai do laco e reenvia 
            if (status == RESP_NACK || status == RESP_CORROMPIDA || status == RESP_TIMEOUT) break;

            // Acontece somente no servidor, quando esta esperando um ACK depois de enviar um OK
            // Caso nao chegue um ACK mas um novo pedido de comando:
            if (eh_pedido(resposta->seq_tipo & 0x0f)) {
                // Se a sequencia for a mesma, o cliente nao recebeu o OK
                // O servidor reenvia o OK sem fazer um CD a mais
                if ( ((resposta->seq_tipo & 0xf0) >> 4) == seq )
                    status = RESP_NACK;
                // Se a sequencia for diferente, entao o cliente mandou outro comando,
                // o que significa que recebeu o OK mas o ACK se perdeu
                else
                    status = RESP_ACK;
                break;
            }

            // Eh a resposta correta deste envio, sendo um ACK ou OK/ERRO
            if (((resposta->seq_tipo & 0xf0) >> 4) == seq) break;

            // Caso a mensagem recebida nao atenda a nenhuma das condicoes, eh descartada
            // Assim, volta a esperar uma resposta correta
        }

        switch (status) {
        case RESP_ACK:          return 0;
        case RESP_OUTRA:        return 1;
        case RESP_NACK:         fprintf(stderr, "NACK, reenviando (%d/%d)\n", envio, TENT_MAX_ENVIOS); break;
        case RESP_CORROMPIDA:   fprintf(stderr, "Resposta corrompida, reenviando (%d/%d)\n", envio, TENT_MAX_ENVIOS); break;
        case RESP_TIMEOUT:      fprintf(stderr, "Timeout, reenviando (%d/%d)\n", envio, TENT_MAX_ENVIOS); break;
        }
    }

    return -1;
}