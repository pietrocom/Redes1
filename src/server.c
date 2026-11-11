#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"
#include "commands.h"

// --- Defines ---

#define DEBUG_PRINT 1   // 1 para mostrar os prints, 0 c.c.


// --- Funcoes estaticas ---

static void debug_prints (struct protocolo pacote_recebido, uint8_t tipo, uint8_t sequencia) {
    printf("Pacote sem erros (paridade compatível)\n");

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
}


int run_server() {

    // Cria socket do servidor
    int soquete = cria_raw_socket("lo");

    while(1) {

        uint8_t mac_servidor[6] = MACSERVIDOR;
        uint8_t mac_client[6]   = MACCLIENTE;
        struct protocolo pacote_recebido;
        memset(&pacote_recebido, 0, sizeof(struct protocolo));
        int recebidos = receive_message(soquete, mac_servidor, &pacote_recebido);


        // Caso tenha recebido uma mensagem com sucesso
        if (recebidos > 0) {
            uint8_t sequencia = (pacote_recebido.seq_tipo >> 4) & 0x0F; 
            uint8_t tipo = pacote_recebido.seq_tipo & 0x0F;

            if (DEBUG_PRINT == 1) debug_prints(pacote_recebido, tipo, sequencia);

            switch (tipo)
            {
            case TIPO_CD:
                cd_servidor(&pacote_recebido, soquete, mac_client, mac_servidor);
                break;

            case TIPO_TEST:
                ack(soquete, mac_client, mac_servidor, sequencia);
                break;
            
            default:
                break;
            } 
        } 
        // Caso a mensagem tenha sido corrompida
        if (recebidos == 0) {
            nack(soquete, mac_client, mac_servidor);
            printf("ERRO! Paridade não é compatível\n");
            continue;
        }
        // Caso tenha havido timeout
        if (recebidos == -1) continue; 
    }

    return 0;
}
