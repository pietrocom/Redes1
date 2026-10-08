#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"

void lcd (char *path) {

    if (chdir(path) == 0) {
        printf("Mudando diretório local para: %s\n", path);
    } else {
        perror("Erro ao mudar de diretório local ");
    }
}

void lls () {
    
    int out = system("ls");

    if (out == -1) {
        perror("Erro ao executar listagem local");
    }
}

int cd (int soquete, char *path, uint8_t *MACDestino, uint8_t *MACOrigem) {

    if (strlen(path) > 117) {
        printf("Tamanho do destino maior do que o permitido.\n");
        return -1;
    }

    struct protocolo mensagem;
    build_message(MACDestino, MACOrigem, 0, 3, path, 0);
}