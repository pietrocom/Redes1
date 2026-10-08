#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#include "input_parser.h"
#include "raw_sockets.h"


// --- Variaveis Globais ---

int server_port;


int main (int argc, char * argv[]) {
    int port = cria_raw_socket("lo");

    char buffer[TAM_MAX_INPUT];

    int running = 1;
    while (running == 1) {
        int qtd_bytes = recv(port, buffer, TAM_MAX_INPUT, 0);

        if (qtd_bytes != 0) {break;}
    }

    printf("%s", buffer);
}