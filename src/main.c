/* 
Trabalho 1 de Redes de Computadores 2026/02. 
Autores: Camila Shibata (GRR) e Pietro Comin (20241955)
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#include "input_parser.h"
#include "raw_sockets.h"

/* 
Entradas:
 - trabalho1
 - interface de rede
*/
int main (int argc, char * argv[]) {
    // Pega as entradas do programa
    if (argc != 2) {
        fprintf(stderr, "Entradas incorretas ao programa.\n Correto eh: sudo ./trabalho1 interface_de_rede\n");
        exit(1);
    }

    int port = cria_raw_socket("enp0s31f6");

    #ifdef CLIENTE
    int status = 1;
    char input [TAM_MAX_INPUT];
    while (status) {
        printf("> ");
        fgets(input, TAM_MAX_INPUT, stdin); printf("\n");

        handle_input(&input);
    }
    #endif
    
    return 0;
}