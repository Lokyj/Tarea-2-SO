#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "matriz_hooks.h"

void print_matriz_simple(Entidad*** matriz, int width, int height, int cantidad_heroes) {
    printf("\n=== VISUALIZACIÓN DE LA MATRIZ ===\n");
    printf("H = Héroe, M = Monstruo, . = Vacío, X = Meta\n\n");
    
    // arreglo que guardara las metas de cada heroe
    Coord metas[cantidad_heroes];
    int metas_count = 0;
    
    // busca las metas de los heroes
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (matriz[i][j] != NULL) {
                if (strcmp(matriz[i][j]->type, "heroe") == 0) {
                    Entidad *entidad_heroe = matriz[i][j];
                    
                    if (entidad_heroe->ruta != NULL && entidad_heroe->ruta_length > 0) {
                        if (metas_count < cantidad_heroes) {
                            metas[metas_count].x = entidad_heroe->ruta[entidad_heroe->ruta_length - 1].x;
                            metas[metas_count].y = entidad_heroe->ruta[entidad_heroe->ruta_length - 1].y;
                            metas_count++;
                        }
                    }
                }
            }
        }
    }
    
    // imprime toda la matriz, junto con las metas recolectadas
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int es_meta = 0;
            
            for (int k = 0; k < metas_count; k++) {
                if (i == metas[k].y && j == metas[k].x) {
                    es_meta = 1;
                    break;
                }
            }
            
            if (matriz[i][j] != NULL) {
                if (strcmp(matriz[i][j]->type, "heroe") == 0) {
                    printf("H ");
                } else if (strcmp(matriz[i][j]->type, "monstruo") == 0) {
                    printf("M ");
                }
            } else if (es_meta > 0) {
                printf("X ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
}

void mover_entidad_matriz(Entidad*** matriz, int from_x, int from_y, int to_x, int to_y, int width, int height) {
    // verifica limites
    if (from_x < 0 || from_x >= width || from_y < 0 || from_y >= height) {
        printf("Error: Posición origen fuera de límites\n");
        return;
    }
    if (to_x < 0 || to_x >= width || to_y < 0 || to_y >= height) {
        printf("Error: Posición destino fuera de límites\n");
        return;
    }
    
    // verifica si hay alguna entidad en la posición de origen
    if (matriz[from_y][from_x] == NULL) {
        printf("Error: No hay entidad en posición origen\n");
        return;
    }
    
    // ver si el destino esta disponible
    if (matriz[to_y][to_x] != NULL) {
        printf("Error: Posición destino ocupada\n");
        return;
    }
    
    // si no esta ocupado, se mueve
    matriz[to_y][to_x] = matriz[from_y][from_x];
    matriz[from_y][from_x] = NULL;
    
    // actualiza las coords de la entidad movida
    matriz[to_y][to_x]->start_coords.x = to_x;
    matriz[to_y][to_x]->start_coords.y = to_y;
}

int distanciaManhattan(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

int generarCamino(int x1, int y1, int x2, int y2, Coord *camino) {
    int indice = 0;
    int x = x1, y = y1;
    
    camino[indice].x = x;
    camino[indice].y = y;
    indice++;
    
    while (x != x2) {
        if (x < x2) x++;
        else x--;
        camino[indice].x = x;
        camino[indice].y = y;
        indice++;
    }
    
    while (y != y2) {
        if (y < y2) y++;
        else y--;
        camino[indice].x = x;
        camino[indice].y = y;
        indice++;
    }

    return indice; // retorna la distancia recorrida
}