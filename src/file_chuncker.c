#include "file_chuncker.h"
#include <stdio.h>

// Retorna o tamanho do arquivo, ou -1 em caso de erro
static int tamanho_do_arquivo (FILE * file) {
    if (fseek(file, 0, SEEK_END) != 0) {
        printf("Erro ao navegar no arquivo.\n");
        return -1;
    }

    return ftell(file);
}

int extrai_chunk (FILE * file, void * chunk, short tam, int pos) {
    if (!file) {
        printf("Arquivo nao foi especificado em para extracao de chunck.\n");
        return -1;
    }
    if (tam <= 0 || tam > CHUNCK_MAX_SIZE) {
        printf("Tamanho especificado para extracao de chunck invalido\n");
        return -1;
    }
    int tamanho_arquivo = tamanho_do_arquivo(file);
    if (tamanho_arquivo == -1) {
        printf("Erro ao ler o tamanho do arquivo.\n");
        return -1;
    }
    if ( (pos + tam) > tamanho_arquivo ) {
        printf("Leitura invalida: fim do arquivo alacancado.\n");
        return -1;
    }

    if (fseek(file, pos, SEEK_SET) != 0) {
        printf("Erro ao navegar no arquivo.\n");
        return -1;
    }
    if (fread(chunk, tam, 1, file) != 1) {
        printf("Erro na leitura do arquivo.\n");
        return -1;
    }

    return 0;
}

