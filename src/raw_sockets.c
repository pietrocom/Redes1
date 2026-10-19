/*
Este arquivo implementa as funcoes que manipulam raw sockets.
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
 
    return soquete;
}

int send_message (int soquete, uint8_t *mac_dest, uint8_t *mac_orig, uint8_t sequencia, uint8_t tipo, void *dados, uint16_t tamanho_dados) {

    // Construcao do pacote
    struct protocolo pacote = build_message(mac_dest, mac_orig, sequencia, tipo, dados, tamanho_dados);

    // Calcula tamanho do pacote
    int tam_envio = 19 + tamanho_dados;
    if (tam_envio < TAM_MINIMO) {
        tam_envio = TAM_MINIMO;
    }

    // Envia pacote
    int enviados = send(soquete, &pacote, tam_envio, 0);
    
    if (enviados == -1) {
        printf("Erro ao enviar mensagem!\n");
    }

    return enviados;
}

int receive_message (int soquete, uint8_t *meu_mac, struct protocolo *pacote_retorno) {

    unsigned char buffer[TAM_MAXIMO];

    while(1) {
        // Recebe e guarda mensagem no buffer
        int recebidos = recv(soquete, buffer, TAM_MAXIMO, 0);

        if (recebidos > 0) {
            struct protocolo *pacote_recebido = (struct protocolo *) buffer;

            // Verifica endereco de destino
            if (memcmp(pacote_recebido->end_destino, meu_mac, 6) == 0) {
                
                // Verifica paridade
                if (calculo_paridade(pacote_recebido, recebidos) == 0) {
                    
                    memcpy(pacote_retorno, pacote_recebido, 19 + pacote_recebido->tamanho);
                    return recebidos;
                    
                } else {
                    return -1; 
                }
            }
        }
    }
}
