#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "input_parser.h"
#include "commands.h"

#define MAX_ARGS 4

enum comando {
    GET,
    PUT,
    LS,
    CD, 
    LLS,
    LCD,
    EXIT,
    INVALIDO
};

// --- Funcoes estaticas ---

static enum comando parse_comando (const char * comando) {
    if (strcmp(comando, "get")  == 0) return GET;
    if (strcmp(comando, "put")  == 0) return PUT;
    if (strcmp(comando, "ls")   == 0) return LS;
    if (strcmp(comando, "cd")   == 0) return CD;
    if (strcmp(comando, "lls")  == 0) return LLS;
    if (strcmp(comando, "lcd")  == 0) return LCD;
    if (strcmp(comando, "exit") == 0) return EXIT;
    return INVALIDO;
}

static int split_input (char *input, char **argv, int max_args) {
    char * save_ptr;    // Usado para salvar o estado apos cada execucao (obs: isso acontece
                        // somente na variante com o _r pois nao usa a variavel estatica da biblioteca)
    char *arg = strtok_r(input, " \t\n", &save_ptr);
    int argc = 0;

    while (arg != NULL && argc < MAX_ARGS) {    // Le enquanto houverem argumentos
        argv[argc] = arg;
        argc++;
        arg = strtok_r(NULL, " \t\n", &save_ptr);
    }

    return argc; // Retorna o numero de argumentos dados
}


// --- Funcoes da API ---

int handle_input (char * input, int soquete, uint8_t *mac_cliente, uint8_t *mac_servidor) {
    if (!input) {
        fprintf(stderr, "Erro ao acessar o input.\n");
        exit (1);
    }

    char * argv[MAX_ARGS];
    int argc = split_input(input, argv, MAX_ARGS);

    char * comando = argv[0];
    if (!comando) return 0; // Linha vazia

    enum comando parsed_comando = parse_comando(comando);
    switch (parsed_comando)
    {
    case INVALIDO:
        fprintf(stderr, "Comando invalido.\n");
        break;

    case GET:
        break;

    case PUT:
        put_cliente(soquete, argv[1], mac_servidor, mac_cliente);
        break;

    case LS:
        int valido = 1;
        for (int i = 1; i < argc; i++)
            if (strcmp(argv[i], "-l") != 0 && strcmp(argv[i], "-a") != 0) {
                fprintf(stderr, "Flag invalida. Uso: ls -l/-a\n");
                valido = 0;
                break;
            }
        if (valido)
            ls_cliente(soquete, argc, argv, mac_servidor, mac_cliente);
        break;

    case CD:
        cd_cliente(soquete, argv[1], mac_servidor, mac_cliente);
        break;

    case LLS:
        lls();
        break;

    case LCD:

        lcd(argv[1]);
        break;

    case EXIT:

        return -1;
    }
    
    return 0;
}