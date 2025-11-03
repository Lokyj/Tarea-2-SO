#ifndef MATRIZ_UTILS_H
#define MATRIZ_UTILS_H

#include "entidades.h"

// funcion para imprimir la matriz
void print_matriz_simple(Entidad*** matriz, int width, int height, int cantidad_heroes);

// Mover una entidad en la matriz
void mover_entidad_matriz(Entidad*** matriz, int from_x, int from_y, int to_x, int to_y, int width, int height);

// funcion para obtener la distancia de manhattan entre dos puntos
int distanciaManhattan(int x1, int y1, int x2, int y2);

// generar camino entre dos puntos y guardarlo en el arreglo camino
int generarCamino(int x1, int y1, int x2, int y2, Coord *camino);

#endif

