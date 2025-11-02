#ifndef PARSER_H
#define PARSER_H

#include "entidades.h"

// Leer archivo de configuración y crear matriz
// Nota: el parámetro `monstruos` es un output (puntero a puntero). La función
// asignará memoria para el arreglo de monstruos y pondrá *monstruos al arreglo.
Entidad ***read_game_config(const char *filename, int *width, int *height, int *total_entidades, Entidad **heroes, Entidad **monstruos, int *cantidad_monstruos, int *cantidad_héroes);

// Liberar memoria de la matriz
void free_matriz(Entidad ***matriz, int height, int width);

#endif