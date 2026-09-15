/*
Esse arquivo cuida da separacao de arquivos em chuncks, estes 
que serao enviados um por um ao servidor, e vice versa. 
*/

#ifndef FILE_CHUNCKER
#define FILE_CHUNCKER

#include <stdio.h>

#define CHUNCK_MAX_SIZE 117

// Extrai o chunck de tamanho tam, a partir do byte pos, do arquivo file.
// Retorna 0 em caso de sucesso e -1 em caso de erro.
int extrai_chunk (FILE * file, void * chunk, short tam, int pos);

#endif