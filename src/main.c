/* 
Trabalho 1 de Redes de Computadores 2026/02. 
Autores: Camila Shibata (GRR) e Pietro Comin (20241955)
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

#define TAM_MAX_INPUT 150

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

    int status = 1;
    char input [TAM_MAX_INPUT];
    while (status) {
        printf("> ");
        fgets(input, TAM_MAX_INPUT, stdin); printf("\n");


    }
    
    return 0;
}