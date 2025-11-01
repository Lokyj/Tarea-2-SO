#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

#define MAX_PATH_LENGTH 1000
#define MAX_MONSTERS 50
#define MAX_LINE 2048

// Función para parsear coordenadas del path del héroe
static int parse_hero_path(char* line, Coord** path) {
    Coord* coords = malloc(MAX_PATH_LENGTH * sizeof(Coord));
    int count = 0;
    char* ptr = line;
    
    while (*ptr != '\0') {
        if (*ptr == '(') {
            int x, y;
            if (sscanf(ptr, "(%d,%d)", &x, &y) == 2) {
                coords[count].x = x;
                coords[count].y = y;
                count++;
            }
            while (*ptr != ')' && *ptr != '\0') ptr++;
            if (*ptr == ')') ptr++;
        } else {
            ptr++;
        }
    }
    
    *path = realloc(coords, count * sizeof(Coord));
    return count;
}

Entidad*** read_game_config(const char* filename, int* width, int* height, int* total_entidades) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s\n", filename);
        return NULL;
    }
    
    char line[MAX_LINE];
    int grid_width = 0, grid_height = 0;
    int monster_count = 0;
    
    // Primera pasada: obtener dimensiones y contar monstruos
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "GRID_SIZE", 9) == 0) {
            sscanf(line, "GRID_SIZE %d %d", &grid_width, &grid_height);
        }
        else if (strncmp(line, "MONSTER_COUNT", 13) == 0) {
            sscanf(line, "MONSTER_COUNT %d", &monster_count);
        }
    }
    
    *width = grid_width;
    *height = grid_height;
    *total_entidades = monster_count + 1;
    
    // Crear matriz inicializada en NULL
    Entidad*** matriz = malloc(grid_height * sizeof(Entidad**));
    for (int i = 0; i < grid_height; i++) {
        matriz[i] = malloc(grid_width * sizeof(Entidad*));
        for (int j = 0; j < grid_width; j++) {
            matriz[i][j] = NULL;
        }
    }
    
    // Crear arreglo temporal de entidades
    Entidad* entidades = malloc(*total_entidades * sizeof(Entidad));
    
    // Inicializar héroe (índice 0)
    strcpy(entidades[0].type, "heroe");
    strcpy(entidades[0].estado, "vivo");
    entidades[0].rango_vision = NULL;
    entidades[0].ruta = NULL;
    entidades[0].ruta_length = 0;
    
    // Inicializar monstruos (índices 1 a n)
    for (int i = 1; i < *total_entidades; i++) {
        strcpy(entidades[i].type, "monstruo");
        strcpy(entidades[i].estado, "vivo");
        entidades[i].ruta = NULL;
        entidades[i].ruta_length = 0;
        entidades[i].rango_vision = malloc(sizeof(int));
        *entidades[i].rango_vision = 0;
    }
    
    // Volver al inicio del archivo para parsear datos
    rewind(file);
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        if (strncmp(line, "HERO_HP", 7) == 0) {
            sscanf(line, "HERO_HP %d", &entidades[0].vida);
        }
        else if (strncmp(line, "HERO_ATTACK_DAMAGE", 18) == 0) {
            sscanf(line, "HERO_ATTACK_DAMAGE %d", &entidades[0].daño);
        }
        else if (strncmp(line, "HERO_ATTACK_RANGE", 17) == 0) {
            sscanf(line, "HERO_ATTACK_RANGE %d", &entidades[0].rango_ataque);
        }
        else if (strncmp(line, "HERO_START", 10) == 0) {
            sscanf(line, "HERO_START %d %d", 
                   &entidades[0].start_coords.x, 
                   &entidades[0].start_coords.y);
        }
        else if (strncmp(line, "HERO_PATH", 9) == 0) {
            char path_buffer[MAX_LINE * 10] = "";
            strcat(path_buffer, line + 10);
            
            long pos = ftell(file);
            while (fgets(line, sizeof(line), file)) {
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) > 0 && line[0] == '(') {
                    strcat(path_buffer, " ");
                    strcat(path_buffer, line);
                } else {
                    fseek(file, pos, SEEK_SET);
                    break;
                }
                pos = ftell(file);
            }
            
            entidades[0].ruta_length = parse_hero_path(path_buffer, &entidades[0].ruta);
        }
        else if (strstr(line, "MONSTER_") == line) {
            int monster_id;
            int value;
            
            if (sscanf(line, "MONSTER_%d_HP %d", &monster_id, &value) == 2) {
                entidades[monster_id].vida = value;
            }
            else if (sscanf(line, "MONSTER_%d_ATTACK_DAMAGE %d", &monster_id, &value) == 2) {
                entidades[monster_id].daño = value;
            }
            else if (sscanf(line, "MONSTER_%d_VISION_RANGE %d", &monster_id, &value) == 2) {
                *entidades[monster_id].rango_vision = value;
            }
            else if (sscanf(line, "MONSTER_%d_ATTACK_RANGE %d", &monster_id, &value) == 2) {
                entidades[monster_id].rango_ataque = value;
            }
            else if (sscanf(line, "MONSTER_%d_COORDS %d %d", &monster_id, 
                           &entidades[monster_id].start_coords.x,
                           &entidades[monster_id].start_coords.y) == 3) {
                // Procesado
            }
        }
    }
    
    fclose(file);
    
    // Colocar entidades en la matriz según sus coordenadas iniciales
    for (int i = 0; i < *total_entidades; i++) {
        int x = entidades[i].start_coords.x;
        int y = entidades[i].start_coords.y;
        
        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) {
            matriz[y][x] = &entidades[i];
        }
    }
    
    return matriz;
}

void free_matriz(Entidad*** matriz, int height, int width) {
    if (!matriz) return;
    
    Entidad** entidades_unicas = malloc(height * width * sizeof(Entidad*));
    int count = 0;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (matriz[i][j] != NULL) {
                bool encontrada = false;
                for (int k = 0; k < count; k++) {
                    if (entidades_unicas[k] == matriz[i][j]) {
                        encontrada = true;
                        break;
                    }
                }
                if (!encontrada) {
                    entidades_unicas[count++] = matriz[i][j];
                }
            }
        }
    }
    
    for (int i = 0; i < count; i++) {
        if (entidades_unicas[i]->ruta) {
            free(entidades_unicas[i]->ruta);
        }
        if (entidades_unicas[i]->rango_vision) {
            free(entidades_unicas[i]->rango_vision);
        }
    }
    
    if (count > 0) {
        free(entidades_unicas[0]);
    }
    
    free(entidades_unicas);
    
    for (int i = 0; i < height; i++) {
        free(matriz[i]);
    }
    free(matriz);
}
