#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"

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

int cd_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig) {

    if (strlen(path) > 117) {
        printf("Tamanho do destino maior do que o permitido.\n");
        return -1;
    }

    struct protocolo * resposta = malloc(sizeof(struct protocolo));
    if (!resposta) {fprintf(stderr, "Erro ao alocar estrutura para resposta do CD.\n"); exit(1);}

    int status = send_and_wait(soquete, mac_dest, mac_orig, 0, TIPO_CD, path, strlen(path), resposta);

    if (status == -1) {
        fprintf(stderr, "Nao foi possivel realizar o comando cd.\n");
        return -1;
    }

    if (status == 1) {
        uint8_t tipo = resposta->seq_tipo & 0x0f;
        switch (tipo)
        {
        case TIPO_OK:
            return 0;
        
        case TIPO_ERRO:
            fprintf(stderr, "Servidor nao mudou de diretorio, diretorio inexistente.\n");
            return -1;
        
        default:
            fprintf(stderr, "Resposta inesperada do tipo %d.\n", tipo);
            return -1;
        }
    }

    return 0;
}