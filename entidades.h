#ifndef ENTIDADES_H
#define ENTIDADES_H

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
    Coord current_coords;
    int* rango_vision;      // NULL para héroe
    Coord* ruta;            // NULL para monstruos
    int ruta_length;
} Entidad;

#endif