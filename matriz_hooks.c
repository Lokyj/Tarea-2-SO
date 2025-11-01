#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "matriz_hooks.h"

void print_matriz(Entidad*** matriz, int width, int height, int total_entidades) {
    printf("=== MATRIZ DEL JUEGO ===\n");
    printf("Dimensiones: %d x %d\n", width, height);
    printf("Total de entidades: %d\n\n", total_entidades);
    
    printf("Entidades en la matriz:\n");
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (matriz[i][j] != NULL) {
                Entidad* e = matriz[i][j];
                printf("\nPosición [%d][%d] (x=%d, y=%d):\n", i, j, j, i);
                printf("  Tipo: %s\n", e->type);
                printf("  Estado: %s\n", e->estado);
                printf("  Vida: %d\n", e->vida);
                printf("  Daño: %d\n", e->daño);
                printf("  Rango de ataque: %d\n", e->rango_ataque);
                
                if (e->rango_vision) {
                    printf("  Rango de visión: %d\n", *e->rango_vision);
                }
                
                if (e->ruta_length > 0 && e->ruta) {
                    printf("  Longitud de ruta: %d\n", e->ruta_length);
                    printf("  Primeras coordenadas: ");
                    for (int k = 0; k < e->ruta_length && k < 3; k++) {
                        printf("(%d,%d) ", e->ruta[k].x, e->ruta[k].y);
                    }
                    printf("\n");
                }
            }
        }
    }
}

void print_matriz_simple(Entidad*** matriz, int width, int height) {
    printf("\n=== VISUALIZACIÓN DE LA MATRIZ ===\n");
    printf("H = Héroe, M = Monstruo, . = Vacío\n\n");
    
    int max_show_x = width > 50 ? 50 : width;
    int max_show_y = height > 30 ? 30 : height;
    
    for (int i = 0; i < max_show_y; i++) {
        for (int j = 0; j < max_show_x; j++) {
            if (matriz[i][j] != NULL) {
                if (strcmp(matriz[i][j]->type, "heroe") == 0) {
                    printf("H ");
                } else {
                    printf("M ");
                }
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
    
    if (width > max_show_x || height > max_show_y) {
        printf("\n(Mostrando solo primeros %dx%d de %dx%d)\n", 
               max_show_x, max_show_y, width, height);
    }
}

void mover_entidad_matriz(Entidad*** matriz, int from_x, int from_y, int to_x, int to_y, int width, int height) {
    // Verificar límites
    if (from_x < 0 || from_x >= width || from_y < 0 || from_y >= height) {
        printf("Error: Posición origen fuera de límites\n");
        return;
    }
    if (to_x < 0 || to_x >= width || to_y < 0 || to_y >= height) {
        printf("Error: Posición destino fuera de límites\n");
        return;
    }
    
    // Verificar que hay una entidad en origen
    if (matriz[from_y][from_x] == NULL) {
        printf("Error: No hay entidad en posición origen\n");
        return;
    }
    
    // Verificar que destino está vacío
    if (matriz[to_y][to_x] != NULL) {
        printf("Error: Posición destino ocupada\n");
        return;
    }
    
    // Mover
    matriz[to_y][to_x] = matriz[from_y][from_x];
    matriz[from_y][from_x] = NULL;
    
    // Actualizar coordenadas de la entidad
    matriz[to_y][to_x]->start_coords.x = to_x;
    matriz[to_y][to_x]->start_coords.y = to_y;
}

Entidad* get_entidad_at(Entidad*** matriz, int x, int y, int width, int height) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return NULL;
    }
    return matriz[y][x];
}

Coord* find_heroe(Entidad*** matriz, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (matriz[i][j] != NULL && strcmp(matriz[i][j]->type, "heroe") == 0) {
                Coord* pos = malloc(sizeof(Coord));
                pos->x = j;
                pos->y = i;
                return pos;
            }
        }
    }
    return NULL;
}