#ifndef MATRIZ_UTILS_H
#define MATRIZ_UTILS_H

#include "entidades.h"

// Imprimir la matriz completa
void print_matriz(Entidad*** matriz, int width, int height, int total_entidades);

// Imprimir visualización simple
void print_matriz_simple(Entidad*** matriz, int width, int height);

// Mover una entidad en la matriz
void mover_entidad_matriz(Entidad*** matriz, int from_x, int from_y, int to_x, int to_y, int width, int height);

// Obtener entidad en posición
Entidad* get_entidad_at(Entidad*** matriz, int x, int y, int width, int height);

// Buscar héroe en la matriz
Coord* find_heroe(Entidad*** matriz, int width, int height);

#endif