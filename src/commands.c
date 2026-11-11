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

// Ao realizar o CD, o cliente espera que o servidor responda com um OK 
// caso bem sucedido.
int cd_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig) {

    if (strlen(path) > 117) {
        printf("Tamanho do destino maior do que o permitido.\n");
        return -1;
    }

    struct protocolo * resposta = malloc(sizeof(struct protocolo));
    if (!resposta) {fprintf(stderr, "Erro ao alocar estrutura para resposta do CD.\n"); exit(1);}

    uint8_t seq = proxima_seq();

    int status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_CD, path, strlen(path), resposta);

    if (status == -1) {
        fprintf(stderr, "Nao foi possivel realizar o comando cd.\n");
        return -1;
    }

    if (status == 1) {
        uint8_t tipo = resposta->seq_tipo & 0x0f;
        switch (tipo)
        {
        case TIPO_OK:
            // Tipo OK requer timeout, por isso cliente responde que recebeu
            send_message(soquete, mac_dest, mac_orig, seq, TIPO_ACK, NULL, 0);
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

int cd_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig) {
    if (!pacote_recebido) {fprintf(stderr, "Erro ao processar pacote recebido: ponteiro invalido."); exit(1);}

    uint16_t tamanho = pacote_recebido->tamanho;
    char caminho [118];     // 117 + \0
    if (tamanho > 117) {printf("Tamanho maior que o esperado.\n"); tamanho = 117;}

    memcpy(caminho, pacote_recebido->dados_crc, tamanho);

    caminho[tamanho] = '\0';

    int status = chdir(caminho);

    struct protocolo resposta;

    uint8_t sequencia = seq(pacote_recebido->seq_tipo);

    if (status == 0) {
        // O servidor espera um ACK do cliente
        int ret = send_and_wait(soquete, mac_dest, mac_orig, sequencia, TIPO_OK, NULL, 0, &resposta);
        if (ret != 0) fprintf(stderr, "Erro ao receber ACK apos CD bem sucedido.\n");
            
        return TIPO_OK;     // A troca de diretorio ocorreu mesmo sem o ACK
    }

    send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
    return TIPO_ERRO;
}