/* 
Trabalho 1 de Redes de Computadores 2026/02. 
Autores: Camila Shibata (GRR) e Pietro Comin (20241955)
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#include "input_parser.h"
#include "raw_sockets.h"


// --- Variaveis Globais ---

int client_port;


/* 
Entradas:
 - cliente
 - interface de rede
*/
int main (int argc, char * argv[]) {
    int port = cria_raw_socket("lo");

    int running = 1;
    char input [TAM_MAX_INPUT] = {123456};
    /*
    while (running) {
        printf("> ");
        fgets(input, TAM_MAX_INPUT, stdin); printf("\n");

        handle_input(&input);
    }
    */
    send(port, input, TAM_MAX_INPUT, 0); 

    return 0;
}