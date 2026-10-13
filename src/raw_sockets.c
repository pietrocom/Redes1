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

int run_server() {

    // Cria socket do servidor
    int soquete = cria_raw_socket("lo");

    // Buffer para chunks
    unsigned char buffer[TAM_MAXIMO];

    while(1) {

        // Guarda mensagem recebida no buffer
        int recebidos = recv(soquete, buffer, TAM_MAXIMO, 0);

        if (recebidos) {

            // Casting do buffer para protocolo
            struct protocolo *pacote_recebido = (struct protocolo *) buffer;
    
            uint8_t sequencia = (pacote_recebido->seq_tipo >> 4) & 0x0F; 
            uint8_t tipo = pacote_recebido->seq_tipo & 0x0F;

            printf("End destino: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                pacote_recebido->end_destino[0], pacote_recebido->end_destino[1], pacote_recebido->end_destino[2], 
                pacote_recebido->end_destino[3], pacote_recebido->end_destino[4], pacote_recebido->end_destino[5]);

            printf("End origem:  %02X:%02X:%02X:%02X:%02X:%02X\n", 
                pacote_recebido->end_origem[0], pacote_recebido->end_origem[1], pacote_recebido->end_origem[2], 
                pacote_recebido->end_origem[3], pacote_recebido->end_origem[4], pacote_recebido->end_origem[5]);

            printf("Tamanho: %d bytes\n",(pacote_recebido->tamanho));
            
            printf("Sequencia: %d\n", sequencia);
            printf("Tipo: %d\n", tipo);
            
            printf("Dados: %s\n", pacote_recebido->dados_crc);

            // Teste de paridade
            if (calculo_paridade(pacote_recebido, recebidos) == 0) {
                printf("Pacote sem erros (paridade compatível)\n");
            } else {
                printf("Pacote corrompido (paridade errada)\n");
            }
        }
    }

    return 0;
}

int run_client() {

    // Cria socket do cliente
    int soquete = cria_raw_socket("lo");

    // Construcao da mensagem
    uint8_t mac_dest[6] = MACSERVIDOR;
    uint8_t mac_orig[6] = MACCLIENTE;

    char * buffer = "mensagem construída pelo cliente";
    int tam_texto = strlen(buffer);
    int tam_envio = 19 + tam_texto;
    if (tam_envio < 64) tam_envio = 64;

    struct protocolo pacote = build_message(mac_dest, mac_orig, 0, 3, buffer, tam_texto);

    // Envio da mensagem
    send(soquete, &pacote, tam_envio, 0);
    printf("Pacote enviado\n");

    char entrada[200];

    while(1) {
        printf(">> ");
        
        // Lê comando no terminal
        fgets(entrada, sizeof(entrada), stdin);

        // Tira quebra de linha do buffer
        entrada[strcspn(entrada, "\n")] = 0;

        if (strlen(entrada) == 0) continue;

        if (strncmp(entrada, "lcd", 3) == 0) {

            // Extrai caminho da entrada
            char *path = entrada + 4;
            lcd(path);

        } else if (strcmp(entrada, "lls") == 0) {
            
            lls();
        
        } else if (strcmp(entrada, "exit") == 0) {
            
            break;
        
        } else printf("Comando não existe\n");
    }

    return 0;
}
