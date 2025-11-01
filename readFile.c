#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PATH_LENGTH 1000
#define MAX_MONSTERS 50
#define MAX_LINE 2048

typedef struct {
    int x;
    int y;
} Coord;

typedef struct {
    char type[50];
    char estado[50];
    int vida;
    int daño;
    int rango_ataque;
    Coord start_coords;
    int* rango_vision;      // NULL para héroe
    Coord* ruta;            // NULL para monstruos
    int ruta_length;
} Entidad;

// Función para parsear coordenadas del path del héroe
int parse_hero_path(char* line, Coord** path) {
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

// Función principal para leer el archivo y crear la matriz
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
        
        // Verificar que las coordenadas estén dentro del grid
        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) {
            // Asignar la entidad a la posición en la matriz
            matriz[y][x] = &entidades[i];
        } else {
            printf("Advertencia: Entidad %d tiene coordenadas fuera del grid (%d, %d)\n", i, x, y);
        }
    }
    
    return matriz;
}

// Función para liberar memoria de la matriz y entidades
void free_matriz(Entidad*** matriz, int height, int width) {
    if (!matriz) return;
    
    // Primero, liberar las entidades (evitando duplicados)
    // Recolectar todas las entidades únicas
    Entidad** entidades_unicas = malloc(height * width * sizeof(Entidad*));
    int count = 0;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (matriz[i][j] != NULL) {
                // Verificar si ya está en la lista
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
    
    // Liberar recursos de cada entidad única
    for (int i = 0; i < count; i++) {
        if (entidades_unicas[i]->ruta) {
            free(entidades_unicas[i]->ruta);
        }
        if (entidades_unicas[i]->rango_vision) {
            free(entidades_unicas[i]->rango_vision);
        }
    }
    
    // Liberar el arreglo de entidades (la primera entidad apunta al inicio del arreglo)
    if (count > 0) {
        free(entidades_unicas[0]);
    }
    
    free(entidades_unicas);
    
    // Liberar la matriz
    for (int i = 0; i < height; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Función para imprimir la matriz (para debugging)
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
    
    printf("\n=== VISUALIZACIÓN DE LA MATRIZ ===\n");
    printf("H = Héroe, M = Monstruo, . = Vacío\n\n");
    
    // Mostrar solo una parte de la matriz si es muy grande
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

// Ejemplo de uso
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }
    
    int width, height, total_entidades;
    Entidad*** matriz = read_game_config(argv[1], &width, &height, &total_entidades);
    
    if (matriz) {
        print_matriz(matriz, width, height, total_entidades);
        
        printf("\n=== EJEMPLO DE ACCESO ===\n");
        printf("Acceder a posición (3, 3):\n");
        if (matriz[3][3] != NULL) {
            printf("  Tipo: %s\n", matriz[3][3]->type);
            printf("  Vida: %d\n", matriz[3][3]->vida);
        } else {
            printf("  Posición vacía\n");
        }
        
        free_matriz(matriz, height, width);
        return 0;
    } else {
        printf("Error al leer la configuración\n");
        return 1;
    }
}