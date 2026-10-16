#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"
#include "commands.h"
#include "input_parser.h"

int run_client() {

    // Cria socket do cliente
    int soquete = cria_raw_socket("lo");

    // Construcao da mensagem
    uint8_t mac_servidor[6] = MACSERVIDOR;
    uint8_t mac_cliente[6] = MACCLIENTE;

    char * buffer = "mensagem construída pelo cliente";
    int enviados = send_message (soquete, mac_servidor, mac_cliente, 0, 3, buffer, strlen(buffer));
    if (enviados) printf("Pacote enviado\n");

    char entrada[200];

    while(1) {
        printf(">> ");
        
        // Lê comando no terminal
        fgets(entrada, sizeof(entrada), stdin);

        int ans = handle_input(entrada, soquete, mac_cliente, mac_servidor);

        if (ans == -1)
            break;
    }

    return 0;
}
