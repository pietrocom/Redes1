#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "input_parser.h"
#include "commands.h"

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

static void split_input (char * input, char ** comando, char ** arg1) {
    char * save_ptr;    // Usado para salvar o estado apos cada execucao (obs: isso acontece
                        // somente na variante com o _r pois nao usa a variavel estatica da biblioteca)
    * comando = strtok_r(input, " \t\n", &save_ptr);
    * arg1    = strtok_r(NULL , " \t\n", &save_ptr);
}


// --- Funcoes da API ---

int handle_input (const char * input, int soquete, uint8_t *mac_cliente, uint8_t *mac_servidor) {
    if (!input) {
        fprintf(stderr, "Erro ao acessar o input.\n");
        exit (1);
    }

    char * comando;
    char * arg1;    // Pode ser que no futuro haja comandos que aceitem mais argumentos

    split_input(input, &comando, &arg1);
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
        break;

    case LS:
        break;

    case CD:

        cd(soquete, arg1, mac_servidor, mac_cliente);
        break;

    case LLS:

        lls();
        break;

    case LCD:

        lcd(arg1);
        break;

    case EXIT:

        return -1;
    }
    
    return 0;
}