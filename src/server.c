#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"

int run_server() {

    // Cria socket do servidor
    int soquete = cria_raw_socket("lo");

    while(1) {

        uint8_t mac_dest[6] = MACSERVIDOR;
        struct protocolo pacote_recebido;
        memset(&pacote_recebido, 0, sizeof(struct protocolo));
        int recebidos = receive_message(soquete, mac_dest, &pacote_recebido);

        if (recebidos != -1) {

            printf("Pacote sem erros (paridade compatível)\n");
            uint8_t sequencia = (pacote_recebido.seq_tipo >> 4) & 0x0F; 
            uint8_t tipo = pacote_recebido.seq_tipo & 0x0F;

            printf("End destino: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                pacote_recebido.end_destino[0], pacote_recebido.end_destino[1], pacote_recebido.end_destino[2], 
                pacote_recebido.end_destino[3], pacote_recebido.end_destino[4], pacote_recebido.end_destino[5]);

            printf("End origem:  %02X:%02X:%02X:%02X:%02X:%02X\n", 
                pacote_recebido.end_origem[0], pacote_recebido.end_origem[1], pacote_recebido.end_origem[2], 
                pacote_recebido.end_origem[3], pacote_recebido.end_origem[4], pacote_recebido.end_origem[5]);

            printf("Tamanho: %d bytes\n",(pacote_recebido.tamanho));
            
            printf("Sequencia: %d\n", sequencia);
            printf("Tipo: %d\n", tipo);
            
            printf("Dados: %s\n", pacote_recebido.dados_crc);
        } else {
            printf("ERRO! Paridade não é compatível\n");
        }
    }

    return 0;
}

