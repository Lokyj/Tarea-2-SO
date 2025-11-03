#ifndef PARSER_H
#define PARSER_H

#include "entidades.h"

Entidad ***read_game_config(const char *filename, int *width, int *height, int *total_entidades, Entidad **heroes, Entidad **monstruos, int *cantidad_monstruos, int *cantidad_héroes);

#endif