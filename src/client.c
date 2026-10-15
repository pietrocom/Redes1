#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"
#include "commands.h"

int run_client() {

    // Cria socket do cliente
    int soquete = cria_raw_socket("lo");

    // Construcao da mensagem
    uint8_t mac_dest[6] = MACSERVIDOR;
    uint8_t mac_orig[6] = MACCLIENTE;

    char * buffer = "mensagem construída pelo cliente";
    int enviados = send_message (soquete, mac_dest, mac_orig, 0, 3, buffer, strlen(buffer));
    if (enviados) printf("Pacote enviado\n");

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
