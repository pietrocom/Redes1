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
 - interface de rede
*/
int main (int argc, char * argv[]) {
    // Pega as entradas do programa
    if (argc != 2) {
        fprintf(stderr, "Entradas incorretas ao programa.\n Correto eh: sudo ./trabalho1 interface_de_rede\n");
        exit(1);
    }

    if (strcmp(argv[1], "cliente") == 0) {

        run_client();
    } else {
        
        run_server();
    }
    
    return 0;
}
