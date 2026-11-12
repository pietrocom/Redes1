#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"
#include "file_chuncker.h"


// --- Funcoes Estaticas ---

// Le de origem em blocos de ate 117 B, envia cada um com send_and_wait
// usando o tipo dado, e no final envia FIM.
static int envia_stream (int soquete, uint8_t * mac_dest, uint8_t * mac_orig, uint8_t tipo, FILE * file, int tamanho) {
    struct protocolo resposta;
    int status;
    
    // Calcula o piso de tamanho / CHUNCK_MAX_SIZE
    int qtd_blocos = tamanho / CHUNCK_MAX_SIZE;

    uint8_t chunk [CHUNCK_MAX_SIZE];
    int seq;

    // Envia os blocos inteiros do arquivo
    for (int i = 0; i < qtd_blocos; i++) {
        status = extrai_chunk(file, &chunk, CHUNCK_MAX_SIZE, i * CHUNCK_MAX_SIZE);
        if (status == -1) {fprintf(stderr, "Erro ao extrair o chunck #%d.\n", i); return -1;}

        seq = proxima_seq();

        status = send_and_wait(soquete, mac_dest, mac_orig, seq, tipo, chunk, CHUNCK_MAX_SIZE, &resposta);

        // Verifica se ocorreu algum erro
        if (status == -1) {fprintf(stderr, "Erro ao enviar o chunck %d.\n", i); return -1;}
        
        // Caso o servidor tenha recebido corretamente, passa para o proximo chunck
    }
    // Envia o restante do arquivo
    int pos   = qtd_blocos * CHUNCK_MAX_SIZE;
    int resto = tamanho - pos;
    if (resto > 0) {
        extrai_chunk(file, &chunk, resto, pos);
        seq = proxima_seq();
        status = send_and_wait(soquete, mac_dest, mac_orig, seq, tipo, chunk, resto, &resposta);
        if (status == -1) {fprintf(stderr, "Erro ao enviar o ultimo chunck"); return -1;}
    }

    // Envia uma mensagem dizendo que o envio do arquivo acabou
    seq = proxima_seq();
    status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_FIM, NULL, 0, &resposta);
    if (status != 0) {fprintf(stderr, "Servidor nao respondeu a ultima mensagem enviada.\n"); return -1;}

    return 0;
}

// Recebe blocos do tipo dado (com o controle de duplicatas acima),
// escreve cada um em destino com fwrite e responde ACK, ate chegar o FIM.
static int recebe_stream (int soquete, uint8_t * meu_mac, uint8_t * outro, uint8_t tipo, FILE * destino) {

}

// Recebe um caminho para um arquivo e retorna o nome do arquivo.
static char * extrai_nome_base (char * path) {
    // Returns a pointer to the last occurrence of character in the C string str.
    // The terminating null-character is considered part of the C string. 
    char * nome = strrchr(path, '/');
    nome = nome ? nome + 1 : path;  // Se for um ponteiro valido, retorna o nome somente (sem o /), 
                                    // senao retorna a caminho em si que ja eh o nome
    return nome;
}



// --- Funcoes da API ---

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

    struct protocolo resposta;
    memset(&resposta, 0, sizeof(struct protocolo));

    uint8_t seq = proxima_seq();

    // Envia a mensagem do CD
    int status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_CD, path, strlen(path), &resposta);

    if (status == -1) {
        fprintf(stderr, "Nao foi possivel realizar o comando cd.\n");
        return -1;
    }

    // Caso a resposta recebida do servidor nao tenha sido ACK nem NACK
    if (status == 1) {
        uint8_t tipo = resposta.seq_tipo & 0x0f;
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

    // Perceba que o caso do ACK ter sido recebido nao eh tratado pois se espera OK apos um CD

    return 0;
}

int cd_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig) {
    if (!pacote_recebido) {fprintf(stderr, "Erro ao processar pacote recebido: ponteiro invalido."); exit(1);}

    // Calcula e instancia o caminho
    uint16_t tamanho = pacote_recebido->tamanho;
    char caminho [118];     // 117 + \0
    if (tamanho > 117) {printf("Tamanho maior que o esperado.\n"); tamanho = 117;}

    memcpy(caminho, pacote_recebido->dados_crc, tamanho);

    // Insere o marcador de fim do caminho
    caminho[tamanho] = '\0';

    int status = chdir(caminho);

    struct protocolo resposta;

    uint8_t sequencia = seq(pacote_recebido->seq_tipo);

    // Se a troca de diretorio foi realizada com sucesso, envia um OK para o cliente
    if (status == 0) {
        // O servidor espera um ACK apos o envio do OK
        int ret = send_and_wait(soquete, mac_dest, mac_orig, sequencia, TIPO_OK, NULL, 0, &resposta);

        // Nesse caso, ou o cliente nao recebeu o OK ou o ACK enviado pelo
        // cliente foi perdido. Em ambos os casos, a troca ocorreu
        if (ret != 0) fprintf(stderr, "Erro ao receber ACK apos CD bem sucedido.\n");
            
        return TIPO_OK;     // A troca de diretorio ocorreu mesmo sem o ACK
    }

    // Se a troca de diretorio nao foi realizada com sucesso, envia uma mensagem de 
    // ERRO ao cliente. Nao espera resposta
    send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
    return TIPO_ERRO;
}

int ls_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig) {
    
    if (strlen(path) > 117) {
        printf("Tamanho do destino maior do que o permitido.\n");
        return -1;
    }

    struct protocolo resposta;
    uint8_t seq = proxima_seq();

    // Envia a mensagem de LS
    int status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_LS, path, strlen(path), &resposta);

    if (status == -1) {
        fprintf(stderr, "Nao foi possivel realizar o comando ls.\n");
        return -1;
    }

    return 0;
}

int ls_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig) {

}

int put_cliente (int soquete, char * path, uint8_t * mac_dest, uint8_t * mac_orig) {
    FILE * file = fopen (path, "rb");
    if (!file) {fprintf(stderr, "Erro ao abrir o arquivo %s. Arquivo inexistente!\n", path); return -1;}

    int tamanho = tamanho_do_arquivo(file);
    if (tamanho == -1) {fprintf(stderr, "Erro ao ler tamanho do arquivo.\n"); fclose (file); return -1;}
    uint32_t t  = tamanho;

    uint8_t seq = proxima_seq();
    struct protocolo resposta;

    // === PRIMEIRA MENSAGEM (PUT) ===
    // A primeira mensagem vai contar ao servidor o tipo da mensagem 
    char * nome = extrai_nome_base(path);
    int status  = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_PUT, nome, strlen(nome), &resposta);
    if (status != 0) {fprintf(stderr, "Erro na comunicacao inicial do PUT.\n"); fclose (file); return -1;}

    seq = proxima_seq();

    // === SEGUNDA MENSAGEM (TAMANHO) ===
    // A segunda mensagem contem o tamanho do arquivo a ser enviado
    status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_TAMANHO, &t, sizeof(t), &resposta);
    if (status == -1) {fprintf(stderr, "Erro ao enviar ao servidor o tamanho do arquivo.\n"); fclose(file); return -1;}
    if (status ==  1) {
        uint8_t tipo = resposta.seq_tipo & 0x0f;
        if (tipo == TIPO_ERRO)  {fprintf(stderr, "Erro: servidor nao aceitou o tamanho do arquivo sendo enviado.\n"); fclose(file); return -1;}
        else                    {fprintf(stderr, "Erro: servidor retornou um tipo de mensagem inesperado.\n"); fclose(file); return -1;}
    }

    // === SEQUENCIA DE MENSAGENS DE DADOS ===
    status = envia_stream(soquete, mac_dest, mac_orig, TIPO_DADOS, file, tamanho);

    fclose(file);
    return status;
}