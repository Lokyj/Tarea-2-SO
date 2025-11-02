#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "matriz_hooks.h"

// hacer que reciba dos arreglos, uno pa heroes y otro pa monstruos, en cada uno guardar las entidades
// que correspondan mientras se lee el archivo de configuracion
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

void print_matriz_simple(Entidad*** matriz, int width, int height, int cantidad_heroes) {
    printf("\n=== VISUALIZACIÓN DE LA MATRIZ ===\n");
    printf("H = Héroe, M = Monstruo, . = Vacío, X = Meta\n\n");
    
    // Array para guardar las metas de cada héroe
    Coord metas[cantidad_heroes];
    int metas_count = 0;
    
    // Primera pasada: recolectar las posiciones de las metas
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
    
    // Segunda pasada: imprimir la matriz
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

int distanciaManhattan(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

int generarCamino(int x1, int y1, int x2, int y2, Coord *camino) {
    int indice = 0;
    int x = x1, y = y1;
    
    camino[indice].x = x;
    camino[indice].y = y;
    indice++;
    
    // Movimiento horizontal
    while (x != x2) {
        if (x < x2) x++;
        else x--;
        camino[indice].x = x;
        camino[indice].y = y;
        indice++;
    }
    
    // Movimiento vertical
    while (y != y2) {
        if (y < y2) y++;
        else y--;
        camino[indice].x = x;
        camino[indice].y = y;
        indice++;
    }
    
    return indice; // Retorna el número de puntos en el camino
}