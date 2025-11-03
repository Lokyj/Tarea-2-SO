#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

#define MAX_PATH_LENGTH 1000
#define MAX_LINE 2048

// funcion para parsear el camino del heroe 
static int parse_hero_path(char *line, Coord **path)
{
    Coord *coords = malloc(MAX_PATH_LENGTH * sizeof(Coord));
    int count = 0;
    char *ptr = line;

    while (*ptr != '\0')
    {
        if (*ptr == '(')
        {
            int x, y;
            if (sscanf(ptr, "(%d,%d)", &x, &y) == 2)
            {
                coords[count].x = x;
                coords[count].y = y;
                count++;
            }
            while (*ptr != ')' && *ptr != '\0')
                ptr++;
            if (*ptr == ')')
                ptr++;
        }
        else
        {
            ptr++;
        }
    }

    *path = realloc(coords, count * sizeof(Coord));
    return count;
}

Entidad ***read_game_config(const char *filename, int *width, int *height, int *total_entidades, Entidad **heroes, Entidad **monstruos, int *cantidad_monstruos, int *cantidad_heroes)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Error: No se pudo abrir el archivo %s\n", filename);
        return NULL;
    }

    char line[MAX_LINE];
    int grid_width = 0, grid_height = 0;
    int monster_count = 0;
    int hero_count = 0;

    // inicialmente, cuenta la cantidad de heroes y monstruos
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "GRID_SIZE", 9) == 0)
        {
            sscanf(line, "GRID_SIZE %d %d", &grid_width, &grid_height);
        }
        else if (strncmp(line, "MONSTER_COUNT", 13) == 0)
        {
            sscanf(line, "MONSTER_COUNT %d", &monster_count);
        }
        else if (strstr(line, "HERO_") == line)
        {
            // contador de heroes
            int hero_id;
            if (sscanf(line, "HERO_%d_", &hero_id) == 1)
            {
                if (hero_id > hero_count)
                {
                    hero_count = hero_id;
                }
            }
            else if (strstr(line, "HERO_HP") ||
                     strstr(line, "HERO_ATTACK_DAMAGE") ||
                     strstr(line, "HERO_ATTACK_RANGE") ||
                     strstr(line, "HERO_START") ||
                     strstr(line, "HERO_PATH"))
            {
                if (hero_count == 0)
                {
                    hero_count = 1;
                }
            }
        }
    }

    *width = grid_width;
    *height = grid_height;
    *cantidad_heroes = hero_count;
    *cantidad_monstruos = monster_count;
    *total_entidades = hero_count + monster_count;

    // crea la matriz inicializada en NULL
    Entidad ***matriz = malloc(grid_height * sizeof(Entidad **));
    for (int i = 0; i < grid_height; i++)
    {
        matriz[i] = malloc(grid_width * sizeof(Entidad *));
        for (int j = 0; j < grid_width; j++)
        {
            matriz[i][j] = NULL;
        }
    }

    Entidad *entidades = malloc(*total_entidades * sizeof(Entidad));

    // inicializar a los heroes 
    for (int i = 0; i < hero_count; i++)
    {
        strcpy(entidades[i].type, "heroe");
        strcpy(entidades[i].estado, "vivo");
        entidades[i].rango_vision = NULL;
        entidades[i].ruta = NULL;
        entidades[i].ruta_length = 0;
    }

    // inicializa a los mostruos
    for (int i = hero_count; i < *total_entidades; i++)
    {
        strcpy(entidades[i].type, "monstruo");
        strcpy(entidades[i].estado, "vivo");
        entidades[i].ruta = NULL;
        entidades[i].ruta_length = 0;
        entidades[i].rango_vision = malloc(sizeof(int));
        *entidades[i].rango_vision = 0;
    }

    // vuelve al inicio del archivo para leer los datos
    rewind(file);

    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0 || line[0] == '#')
            continue;

        // parsea los datos del heroe
        if (strstr(line, "HERO_") == line)
        {
            int hero_id;
            int value;

            // caso 1. Mas de un heroe
            if (sscanf(line, "HERO_%d_HP %d", &hero_id, &value) == 2)
            {
                entidades[hero_id - 1].vida = value;
            }
            else if (sscanf(line, "HERO_%d_ATTACK_DAMAGE %d", &hero_id, &value) == 2)
            {
                entidades[hero_id - 1].daño = value;
            }
            else if (sscanf(line, "HERO_%d_ATTACK_RANGE %d", &hero_id, &value) == 2)
            {
                entidades[hero_id - 1].rango_ataque = value;
            }
            else if (sscanf(line, "HERO_%d_START %d %d", &hero_id,
                            &entidades[hero_id - 1].start_coords.x,
                            &entidades[hero_id - 1].start_coords.y) == 3)
            {
            }
            else if (strstr(line, "_PATH") != NULL && sscanf(line, "HERO_%d_PATH", &hero_id) == 1)
            {
                char path_buffer[MAX_LINE * 10] = "";
                char *path_start = strchr(line, '(');
                if (path_start != NULL)
                {
                    strcpy(path_buffer, path_start);
                }
                else
                {
                    path_buffer[0] = '\0';
                }

                long pos = ftell(file);
                while (fgets(line, sizeof(line), file))
                {
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0 && line[0] == '(')
                    {
                        strcat(path_buffer, " ");
                        strcat(path_buffer, line);
                    }
                    else
                    {
                        fseek(file, pos, SEEK_SET);
                        break;
                    }
                    pos = ftell(file);
                }

                int path_len = parse_hero_path(path_buffer, &entidades[hero_id - 1].ruta);
                entidades[hero_id - 1].ruta_length = path_len;
            }

            // caso 2. Un solo heroe
            else if (sscanf(line, "HERO_HP %d", &value) == 1)
            {
                entidades[0].vida = value; 
            }
            else if (sscanf(line, "HERO_ATTACK_DAMAGE %d", &value) == 1)
            {
                entidades[0].daño = value;
            }
            else if (sscanf(line, "HERO_ATTACK_RANGE %d", &value) == 1)
            {
                entidades[0].rango_ataque = value;
            }
            else if (sscanf(line, "HERO_START %d %d",
                            &entidades[0].start_coords.x,
                            &entidades[0].start_coords.y) == 2)
            {
            }
            else if (strncmp(line, "HERO_PATH", 9) == 0)
            {
                char path_buffer[MAX_LINE * 10] = "";
                char *path_start = strchr(line, '(');
                if (path_start != NULL)
                {
                    strcpy(path_buffer, path_start);
                }
                else
                {
                    path_buffer[0] = '\0';
                }

                long pos = ftell(file);
                while (fgets(line, sizeof(line), file))
                {
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0 && line[0] == '(')
                    {
                        strcat(path_buffer, " ");
                        strcat(path_buffer, line);
                    }
                    else
                    {
                        fseek(file, pos, SEEK_SET);
                        break;
                    }
                    pos = ftell(file);
                }

                int path_len = parse_hero_path(path_buffer, &entidades[0].ruta);
                entidades[0].ruta_length = path_len;
            }
        }

        // parsea los datos del monstruo
        else if (strstr(line, "MONSTER_") == line)
        {
            int monster_id;
            int value;

            if (sscanf(line, "MONSTER_%d_HP %d", &monster_id, &value) == 2)
            {
                entidades[hero_count + monster_id - 1].vida = value;
            }
            else if (sscanf(line, "MONSTER_%d_ATTACK_DAMAGE %d", &monster_id, &value) == 2)
            {
                entidades[hero_count + monster_id - 1].daño = value;
            }
            else if (sscanf(line, "MONSTER_%d_VISION_RANGE %d", &monster_id, &value) == 2)
            {
                *entidades[hero_count + monster_id - 1].rango_vision = value;
            }
            else if (sscanf(line, "MONSTER_%d_ATTACK_RANGE %d", &monster_id, &value) == 2)
            {
                entidades[hero_count + monster_id - 1].rango_ataque = value;
            }
            else if (sscanf(line, "MONSTER_%d_COORDS %d %d", &monster_id,
                            &entidades[hero_count + monster_id - 1].start_coords.x,
                            &entidades[hero_count + monster_id - 1].start_coords.y) == 3)
            {
            }
        }
    }

    fclose(file);

    // guarda a los heroes dentro del arreglo pasado por parametro
    *heroes = malloc(hero_count * sizeof(Entidad));
    for (int i = 0; i < hero_count; i++)
    {
        (*heroes)[i] = entidades[i];
        (*heroes)[i].current_coords = (*heroes)[i].start_coords;
    }

    // guarda a los monstruos dentro del arreglo pasado por parametro
    *monstruos = malloc(monster_count * sizeof(Entidad));
    for (int i = 0; i < monster_count; i++)
    {
        (*monstruos)[i] = entidades[hero_count + i];
        (*monstruos)[i].current_coords = (*monstruos)[i].start_coords;
    }

    // colocar las entidades dentro del espacio que les corresponde en la matriz
    for (int i = 0; i < *total_entidades; i++)
    {
        int x = entidades[i].start_coords.x;
        int y = entidades[i].start_coords.y;

        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height)
        {
            matriz[y][x] = &entidades[i];
        }
    }

    return matriz;
}