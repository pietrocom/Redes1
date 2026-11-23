/* 
Trabalho 1 de Redes de Computadores 2026/02. 
Autores: Camila Shibata (GRR) e Pietro Comin (20241955)
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

#include "raw_sockets.h"
#include "client.h"
#include "server.h"

/*
Entradas:
 - trabalho1
 - papel: cliente ou servidor
 - interface de rede (ex: enp3s0, lo)
*/
int main (int argc, char * argv[]) {
    // Pega as entradas do programa
    if (argc != 3) {
        fprintf(stderr, "Entradas incorretas ao programa.\n Correto eh: sudo ./trabalho1 <cliente|servidor> <interface_de_rede>\n");
        exit(1);
    }

    char * interface = argv[2];

    if (strcmp(argv[1], "cliente") == 0) {
        run_client(interface);
    } else if (strcmp(argv[1], "servidor") == 0) {
        run_server(interface);
    } else {
        fprintf(stderr, "Papel invalido: '%s'. Use 'cliente' ou 'servidor'.\n", argv[1]);
        exit(1);
    }
    
    return 0;
}
