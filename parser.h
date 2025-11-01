#ifndef PARSER_H
#define PARSER_H

#include "entidades.h"

// Leer archivo de configuración y crear matriz
Entidad*** read_game_config(const char* filename, int* width, int* height, int* total_entidades);

// Liberar memoria de la matriz
void free_matriz(Entidad*** matriz, int height, int width);

#endif