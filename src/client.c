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

    // Construcao dos enderecos
    uint8_t mac_servidor[6] = MACSERVIDOR;
    uint8_t mac_cliente[6]  = MACCLIENTE;

    // Mensagem inicial
    char * buffer = "mensagem construída pelo cliente";
    int resposta = send_and_wait(soquete, mac_servidor, mac_cliente, 0, TIPO_TEST, buffer, strlen(buffer), NULL);

    if (resposta == 0) printf("conexao inicial bem sucedida.\n");
    else               {printf("conexao inicial mal sucedida.\n"); exit(1);}

    char entrada[TAM_MAX_INPUT];

    while(1) {
        printf(">> ");
        
        // Lê comando no terminal
        if (!fgets(entrada, sizeof(entrada), stdin)) return -1;

        int ans = handle_input(entrada, soquete, mac_cliente, mac_servidor);

        if (ans == -1)
            break;
    }

    return 0;
}
