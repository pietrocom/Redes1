#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "raw_sockets.h"
#include "file_chuncker.h"


// --- Funcoes Estaticas ---

// Envia o arquivo inteiro: primeiro o TAMANHO, depois os DADOS em blocos de ate
// CHUNCK_MAX_SIZE bytes e por ultimo o FIM. Cada mensagem espera resposta (send_and_wait).
// Retorno: 0 em caso de sucesso; -1 c.c. (inclusive se o receptor recusar o tamanho).
static int envia_stream (int soquete, uint8_t * mac_dest, uint8_t * mac_orig, FILE * file, int tamanho, uint8_t tipo) {
    struct protocolo resposta;
    int status;

    // === MENSAGEM DE TAMANHO ===
    uint32_t t = tamanho;
    status = send_and_wait(soquete, mac_dest, mac_orig, proxima_seq(), TIPO_TAMANHO, &t, sizeof(t), &resposta);
    if (status == -1) {fprintf(stderr, "Erro ao enviar o tamanho do arquivo.\n"); return -1;}
    if (status ==  1) {
        if ((resposta.seq_tipo & 0x0f) == TIPO_ERRO) fprintf(stderr, "Receptor nao aceitou o tamanho do arquivo.\n");
        else                                         fprintf(stderr, "Resposta inesperada ao tamanho do arquivo.\n");
        return -1;
    }

    // === MENSAGENS DE DADOS ===
    // Calcula o piso de tamanho / CHUNCK_MAX_SIZE
    int qtd_blocos = tamanho / CHUNCK_MAX_SIZE;
    uint8_t chunk [CHUNCK_MAX_SIZE];

    // Envia os blocos inteiros do arquivo
    for (int i = 0; i < qtd_blocos; i++) {
        status = extrai_chunk(file, &chunk, CHUNCK_MAX_SIZE, i * CHUNCK_MAX_SIZE);
        if (status == -1) {fprintf(stderr, "Erro ao extrair o chunck #%d.\n", i); return -1;}

        status = send_and_wait(soquete, mac_dest, mac_orig, proxima_seq(), tipo, chunk, CHUNCK_MAX_SIZE, &resposta);

        // Verifica se ocorreu algum erro
        if (status == -1) {fprintf(stderr, "Erro ao enviar o chunck %d.\n", i); return -1;}
        
        // Caso o servidor tenha recebido corretamente, passa para o proximo chunck
    }
    // Envia o restante do arquivo
    int pos   = qtd_blocos * CHUNCK_MAX_SIZE;
    int resto = tamanho - pos;
    if (resto > 0) {
        extrai_chunk(file, &chunk, resto, pos);
        status = send_and_wait(soquete, mac_dest, mac_orig, proxima_seq(), tipo, chunk, resto, &resposta);
        if (status == -1) {fprintf(stderr, "Erro ao enviar o ultimo chunck"); return -1;}
    }

    // === MENSAGEM DE FIM ===
    status = send_and_wait(soquete, mac_dest, mac_orig, proxima_seq(), TIPO_FIM, NULL, 0, &resposta);
    if (status != 0) {fprintf(stderr, "Servidor nao respondeu a ultima mensagem enviada.\n"); return -1;}

    return 0;
}

// Espera a proxima mensagem NOVA, resolvendo sozinha os casos em que nao ha nada
// a processar:
//   - timeout:    tenta de novo, ate TENT_MAX_ENVIOS timeouts;
//   - corrompida: envia NACK para o emissor reenviar;
//   - repetida (mesma sequencia de ultima_seq): o emissor nao recebeu o nosso ACK,
//     entao o ACK eh reenviado e a mensagem eh descartada, evitando processamento duplo.
// ultima_seq: sequencia da ultima mensagem ja confirmada (-1 se nenhuma).
// Retorno: 0 com a mensagem nova em msg; -1 se estourar os timeouts.
static int recebe_nova (int soquete, uint8_t * meu_mac, uint8_t * outro,
                        int ultima_seq, struct protocolo * msg) {
    int status;
    int timeouts = 0;
    while (1) {
        status = receive_message(soquete, meu_mac, msg);
        // Caso tenha dado timeout
        if (status == -1) {
            fprintf(stderr, "Recepcao deu timeout, tentando novamente %d/%d.\n", ++timeouts, TENT_MAX_ENVIOS);
            if (timeouts == TENT_MAX_ENVIOS) return -1;
            continue;
        }
        // Caso chegue corrompida, pede o reenvio
        if (status == 0)  {
            fprintf(stderr, "Mensagem corrompida.\n");
            nack(soquete, outro, meu_mac);
            continue;
        }
        // Caso seja repetida, reenvia o ACK que se perdeu e descarta a mensagem
        if (seq(msg->seq_tipo) == ultima_seq) {
            ack(soquete, outro, meu_mac, ultima_seq);
            continue;
        }
        return 0;
    }
}

// Recebe um stream: primeiro o TAMANHO, depois os blocos do tipo dado (gravados em
// destino) e por ultimo o FIM. Cada mensagem nova eh confirmada com ACK;
// as repetidas sao confirmadas de novo e descartadas.
// Retorno: 0 em caso de sucesso; -1 c.c.
static int recebe_stream (int soquete, uint8_t * meu_mac, uint8_t * outro, uint8_t ultima_seq, uint8_t tipo, FILE * destino) {

    struct protocolo resposta; 

    // === RECEBE MENSAGEM DE TAMANHO ===
    while (1) {
        if (recebe_nova(soquete, meu_mac, outro, ultima_seq, &resposta) == -1) return -1;
        if ((resposta.seq_tipo & 0x0f) == TIPO_TAMANHO && resposta.tamanho == sizeof(uint32_t)) break;
        printf("Warning: mensagem recebida nao eh do tipo tamanho, aguardando mensagem do tipo correto...\n");
    }

    uint32_t tamanho;
    memcpy(&tamanho, resposta.dados_crc, sizeof(tamanho));  // Pega exatamente o tamanho mandado
    ultima_seq = seq(resposta.seq_tipo);
    ack(soquete, outro, meu_mac, ultima_seq);

    // === RECEBE MENSAGENS DE DADOS ===
    uint32_t qtd_blocos = (tamanho % CHUNCK_MAX_SIZE != 0)
                        ? tamanho / CHUNCK_MAX_SIZE + 1 : tamanho / CHUNCK_MAX_SIZE;  // Teto
    for (uint32_t i = 0; i < qtd_blocos; i++) {
        while (1) {
            if (recebe_nova(soquete, meu_mac, outro, ultima_seq, &resposta) == -1) return -1;
            if ((resposta.seq_tipo & 0x0f) == tipo) break;
            printf("Warning: mensagem recebida nao eh do tipo de dados esperado, aguardando mensagem do tipo correto...\n");
        }
        // Escreve o bloco recebido no arquivo 
        if (fwrite(resposta.dados_crc, 1, resposta.tamanho, destino) != resposta.tamanho) {
            fprintf(stderr, "Erro ao escrever no arquivo indicado: tamanho escrito nao bate com o tamanho dos dados da mensagem.\n");
            return -1;
        }
        // So confirma depois de escrever
        ultima_seq = seq(resposta.seq_tipo);
        ack(soquete, outro, meu_mac, ultima_seq);
    }

    // === RECEBE MENSAGEM DE FIM ===
    while (1) {
        if (recebe_nova(soquete, meu_mac, outro, ultima_seq, &resposta) == -1) return -1;
        if ((resposta.seq_tipo & 0x0f) == TIPO_FIM) break;
        printf("Warning: mensagem recebida nao eh do tipo fim, aguardando mensagem do tipo correto...\n");
    }
    ultima_seq = seq(resposta.seq_tipo);
    ack(soquete, outro, meu_mac, ultima_seq);

    // Se o ACK do FIM se perder, o emissor reenvia o FIM, mas ninguem estaria ouvindo.
    // Por isso continua ouvindo, confirmando de novo cada FIM repetido, ate passarem
    // 2 timeouts seguidos sem nada chegar. 
    int status;
    int silencios = 0;
    while (silencios < 2) {
        status = receive_message(soquete, meu_mac, &resposta);
        if (status == -1) { silencios++; continue; }
        silencios = 0;
        if (status > 0 && seq(resposta.seq_tipo) == ultima_seq) ack(soquete, outro, meu_mac, ultima_seq);
    }

    return 0;
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

int cd_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig) {

    struct protocolo resposta;
    memset(&resposta, 0, sizeof(struct protocolo));

    uint8_t seq = proxima_seq();

    // === PRIMEIRA MENSAGEM (CD) ===
    int status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_CD, NULL, 0, &resposta);

    if (status == -1) { fprintf(stderr, "Nao foi possivel realizar o comando cd.\n"); return -1; }

    // === MENSAGENS DO CAMINHO (DADOS) ===
    FILE * file = tmpfile();
    if (!file) { fprintf(stderr, "Erro ao criar arquivo temportario.\n"); return -1; }

    // Escreve o caminho no arquivo temporario para que seja compativel com envia_stream
    fputs(path, file);
    int tam = tamanho_do_arquivo(file);
    if (tam < 0 || (size_t) tam != strlen(path)) {
        fprintf(stderr, "Erro: tamanho do arquivo temporario nao bate com o do caminho.\n");
        fclose(file);
        return -1;
    }

    status = envia_stream(soquete, mac_dest, mac_orig, file, strlen(path), TIPO_DADOS);

    fclose(file);   // Arquivo temporario apagado automaticamente
    return status;
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

int ls_cliente (int soquete, int argc, char **argv, uint8_t *mac_dest, uint8_t *mac_orig) {
    
    struct protocolo resposta;
    memset(&resposta, 0, sizeof(struct protocolo));

    // === MENSAGEM LS COM FLAGS ===

    uint8_t seq = proxima_seq();

    uint8_t flags = 0;
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-a") == 0)
            flags |= (1 << 0);          // flag -a: 000 001 (|: OU bitwise)
        else if  (strcmp(argv[i], "-l") == 0)
            flags |= (1 << 1);          // flag -l: 000 010
    }

    int status;
    if (flags > 0)
        status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_LS, &flags, 1, &resposta);
    else
        status = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_LS, NULL, 0, &resposta);
    
    if (status != 0) {
        fprintf(stderr, "Erro ao enviar comando ls para o servidor.\n");
        return TIPO_ERRO;
    }

    // === RECEBE A RESPOSTA ===

    status = recebe_stream(soquete, mac_orig, mac_dest, seq, TIPO_MOSTRAR, stdout);
    fflush(stdout);

    return status;
}

int ls_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig) {
    (void) pacote_recebido; (void) soquete; (void) mac_dest; (void) mac_orig;
    
    uint8_t  sequencia = seq(pacote_recebido->seq_tipo);
    uint8_t flags = *pacote_recebido->dados_crc;

    // Decodifica as flags
    int a = (flags & (1 << 0)) != 0;
    int f = (flags & (1 << 1)) != 0;

    // Comando a ser executado no servidor
    char cmd[16] = "ls";
    if (a) strcat(cmd, " -a");
    if (f) strcat(cmd, " -l");
    else   strcat(cmd, " -C");  // Forca formatacao em colunas

    // Executa o comando e captura a saida dele
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao executar comando ls no servidor.\n");
        send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
        return -1;
    }

    FILE *tmp = tmpfile();  // Arquivo temporario para armazenas saida do comando
    if (!tmp) {
        perror("Erro ao criar arquivo temporario para ls");
        pclose(fp);
        send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
        return -1;
    }

    // Copia a saida do comando para o arquivo temporario
    char buffer[CHUNCK_MAX_SIZE];
    size_t lidos;
    while ((lidos = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        fwrite(buffer, 1, lidos, tmp);
    }
    pclose(fp);

    int tamanho = tamanho_do_arquivo(tmp);

    ack(soquete, mac_dest, mac_orig, sequencia);

    int status = envia_stream(soquete, mac_dest, mac_orig, tmp, tamanho, TIPO_MOSTRAR);

    fclose(tmp);
    return status;
}

int put_cliente (int soquete, char * path, uint8_t * mac_dest, uint8_t * mac_orig) {
    FILE * file = fopen (path, "rb");
    if (!file) {fprintf(stderr, "Erro ao abrir o arquivo %s. Arquivo inexistente!\n", path); return -1;}

    int tamanho = tamanho_do_arquivo(file);
    if (tamanho == -1) {fprintf(stderr, "Erro ao ler tamanho do arquivo.\n"); fclose (file); return -1;}

    uint8_t seq = proxima_seq();
    struct protocolo resposta;

    // === PRIMEIRA MENSAGEM (PUT) ===
    // A primeira mensagem vai contar ao servidor o tipo da mensagem 
    char * nome = extrai_nome_base(path);
    int status  = send_and_wait(soquete, mac_dest, mac_orig, seq, TIPO_PUT, nome, strlen(nome), &resposta);
    if (status != 0) {fprintf(stderr, "Erro na comunicacao inicial do PUT.\n"); fclose (file); return -1;}

    seq = proxima_seq();

    // === SEQUENCIA DE MENSAGENS ===
    status = envia_stream(soquete, mac_dest, mac_orig, file, tamanho, TIPO_DADOS);

    fclose(file);
    return status;
}

int put_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig) {
    
    uint8_t  sequencia = seq(pacote_recebido->seq_tipo);
    uint16_t tamanho   = pacote_recebido->tamanho;

    if (tamanho == 0 || tamanho > CHUNCK_MAX_SIZE) {
        fprintf(stderr, "Nome de arquivo invalido no PUT.\n");
        send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
        return -1;
    }
    char nome [CHUNCK_MAX_SIZE + 1];
    memcpy(nome, pacote_recebido->dados_crc, tamanho);
    nome[tamanho] = '\0';

    FILE * file = fopen(nome, "wb");   // Cria ou substitui!
    if (!file) {
        fprintf(stderr, "Erro ao criar arquivo no servidor\n");
        send_message(soquete, mac_dest, mac_orig, sequencia, TIPO_ERRO, NULL, 0);
        return -1;
    }

    ack(soquete, mac_dest, mac_orig, sequencia);   // Confirma o PUT

    // Recebe os blocos de dados e salva no arquivo
    int status = recebe_stream(soquete, mac_orig, mac_dest, sequencia, TIPO_DADOS, file);
    fclose(file);
    if (status == -1) remove(nome);   // Caso haja qualquer erro, apaga o arquivo por completo
    return status;
}