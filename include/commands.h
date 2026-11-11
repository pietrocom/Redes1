#ifndef COMMANDS
#define COMMANDS

#include <stdint.h>
#include "protocol.h"

// cd local (cliente)
void lcd (char *path);

// ls local (cliente)
void lls ();

// cd (cliente)
int cd_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig);

// cd (servidor)
int cd_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig);

// ls (cliente)
int ls_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig);

// ls (servidor)
int ls_servidor (struct protocolo * pacote_recebido, int soquete, uint8_t *mac_dest, uint8_t *mac_orig);

// put (cliente)
int put_cliente (int soquete, char * path, uint8_t * mac_dest, uint8_t * mac_orig);


#endif