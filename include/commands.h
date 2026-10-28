#ifndef COMMANDS
#define COMMANDS

// cd local (cliente)
void lcd (char *path);

// ls local (cliente)
void lls ();

// cd (servidor)
int cd_cliente (int soquete, char *path, uint8_t *mac_dest, uint8_t *mac_orig);

#endif