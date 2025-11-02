#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "entidades.h"
#include "parser.h"
#include "matriz_hooks.h"

// tablero
Entidad ***matriz;
int width, height, total_entidades;
int cantidad_monstruos;
int cantidad_heroes;

// entidades
Entidad *heroes;
Entidad *monstruos;

// Semáforos para controlar turnos
sem_t *turno_heroe;
sem_t *turno_monstruos;

// Semáforos para señalar finalización
sem_t *heroe_done;
sem_t *monstruo_done;

pthread_mutex_t mutex;

void inicializar_estructuras_dinamicas()
{
    // asignar memoria para los semáforos de turno de héroes
    turno_heroe = (sem_t *)malloc(cantidad_heroes * sizeof(sem_t));
    if (turno_heroe == NULL)
    {
        fprintf(stderr, "Error: No se pudo asignar memoria para turno_heroe\n");
        free(heroes);
        exit(EXIT_FAILURE);
    }

    // asignar memoria para los semáforos de done de héroes
    heroe_done = (sem_t *)malloc(cantidad_heroes * sizeof(sem_t));
    if (heroe_done == NULL)
    {
        fprintf(stderr, "Error: No se pudo asignar memoria para heroe_done\n");
        free(heroes);
        free(turno_heroe);
        exit(EXIT_FAILURE);
    }

    // inicializar los semáforos
    for (int i = 0; i < cantidad_heroes; i++)
    {
        sem_init(&turno_heroe[i], 0, 0);
        sem_init(&heroe_done[i], 0, 0);
    }

    // Asignar memoria para los semáforos de turno
    turno_monstruos = (sem_t *)malloc(cantidad_monstruos * sizeof(sem_t));
    if (turno_monstruos == NULL)
    {
        fprintf(stderr, "Error: No se pudo asignar memoria para turno_monstruos\n");
        free(monstruos);
        exit(EXIT_FAILURE);
    }

    // Asignar memoria para los semáforos de done
    monstruo_done = (sem_t *)malloc(cantidad_monstruos * sizeof(sem_t));
    if (monstruo_done == NULL)
    {
        fprintf(stderr, "Error: No se pudo asignar memoria para monstruo_done\n");
        free(monstruos);
        free(turno_monstruos);
        exit(EXIT_FAILURE);
    }

    // Inicializar los semáforos
    for (int i = 0; i < cantidad_monstruos; i++)
    {
        sem_init(&turno_monstruos[i], 0, 0);
        sem_init(&monstruo_done[i], 0, 0);
    }

    printf("Estructuras dinámicas inicializadas para %d monstruos\n", cantidad_monstruos);
    printf("Estructuras dinámicas inicializadas para %d héroes\n", cantidad_heroes);
}

void *agente(void *arg)
{
    while (true)
    {
        // Turno de cada héroe
        for (int i = 0; i < cantidad_heroes; i++)
        {
            sem_post(&turno_heroe[i]);
            sem_wait(&heroe_done[i]);
        }

        // Turno de cada monstruo
        for (int i = 0; i < cantidad_monstruos; i++)
        {
            sem_post(&turno_monstruos[i]);
            sem_wait(&monstruo_done[i]);
        }

        // revisar si todos los monstruos o heroes estan muertos
        int heroes_vivos = 0;
        int heroes_que_han_finalizado = 0;
        for (int i = 0; i < cantidad_heroes; i++)
        {
            if (heroes[i].vida > 0)
            {
                heroes_vivos++;
            }
            if (heroes[i].estado[0] == 'f')
            {
                heroes_que_han_finalizado++;
            }
        }

        if (heroes_vivos == 0)
        {
            printf("Todos los héroes han sido derrotados! Fin del juego.\n");
            exit(0);
        }
        if (heroes_que_han_finalizado == cantidad_heroes)
        {
            printf("Todos los héroes han llegado a la meta! Fin del juego.\n");
            exit(0);
        }

        int muertos = cantidad_heroes - heroes_vivos;
        if (muertos + heroes_que_han_finalizado == cantidad_heroes)
        {
            printf("Murieron %d héroes y los demás llegaron a la meta! Fin del juego.\n", muertos);
            exit(0);
        }

        print_matriz_simple(matriz, width, height, cantidad_heroes);
    }
    return NULL;
}

void *heroe(void *arg)
{
    int id = *(int *)arg;

    while (true)
    {
        sem_wait(&turno_heroe[id]);

        pthread_mutex_lock(&mutex);

        // Lógica del héroe
        // printf("Héroe actuando\n");

        int rango_ataque = heroes[id].rango_ataque ? heroes[id].rango_ataque : 0;

        // buscar el monstruo mas cercano
        Coord heroe_pos = heroes[id].current_coords;
        int min_distancia = width + height; // distancia máxima posible
        int closest_monstruo_id = -1;
        for (int i = 0; i < cantidad_monstruos; i++)
        {
            if (monstruos[i].vida > 0)
            {
                Coord monstruo_pos = monstruos[i].current_coords;
                int distancia = distanciaManhattan(heroe_pos.x, heroe_pos.y, monstruo_pos.x, monstruo_pos.y);
                if (distancia < min_distancia)
                {
                    min_distancia = distancia;
                    closest_monstruo_id = i;
                }
            }
        }
        if (closest_monstruo_id != -1)
        {
            // le pega
            if (min_distancia <= rango_ataque)
            {
                monstruos[closest_monstruo_id].vida -= heroes[id].daño;
                if (monstruos[closest_monstruo_id].vida <= 0)
                {
                    printf("Monstruo %d ha sido derrotado!\n", closest_monstruo_id);

                    // eliminar de la matriz y del arreglo de monstruos
                    matriz[monstruos[closest_monstruo_id].current_coords.y][monstruos[closest_monstruo_id].current_coords.x] = NULL;
                    monstruos[closest_monstruo_id] = (Entidad){0}; // Reiniciar entidad
                }
                else
                {
                    printf("Vida restante del monstruo %d: %d\n", closest_monstruo_id, monstruos[closest_monstruo_id].vida);
                }
            }
            else
            {
                //printf("Monstruo %d está fuera de rango de ataque. Avanzando...\n", closest_monstruo_id);

                // si no puede pegarle, el heroe se mueve
                if (heroes[id].ruta_length > 0 && heroes[id].ruta)
                {
                    // mover al heroe a la siguiente coordenada en su ruta
                    Coord next_pos = heroes[id].ruta[0];

                    // actualizar matriz
                    mover_entidad_matriz(matriz, heroes[id].current_coords.x, heroes[id].current_coords.y, next_pos.x, next_pos.y, width, height);

                    // actualizar coordenadas actuales
                    heroes[id].current_coords = next_pos;

                    // desplazar la ruta
                    for (int i = 1; i < heroes[id].ruta_length; i++)
                    {
                        heroes[id].ruta[i - 1] = heroes[id].ruta[i];
                    }
                    heroes[id].ruta_length--;
                    // printf("Héroe se mueve a (%d, %d)\n", next_pos.x, next_pos.y);
                }
                else if (heroes[id].ruta_length == 0)
                {
                    printf("Llegó a su destino\n");
                    heroes[id].estado[0] = 'f'; // 'f' de finalizado
                }
                else
                {
                    printf("Héroe no tiene ruta para moverse\n");
                }
            }
        }
        else
        {
            printf("No hay monstruos disponibles\n");

            // si no hay monstruos, el heroe se mueve
            if (heroes[id].ruta_length > 0 && heroes[id].ruta)
            {
                // mover al heroe a la siguiente coordenada en su ruta
                Coord next_pos = heroes[id].ruta[0];

                // actualizar matriz
                mover_entidad_matriz(matriz, heroes[id].current_coords.x, heroes[id].current_coords.y, next_pos.x, next_pos.y, width, height);

                // actualizar coordenadas actuales
                heroes[id].current_coords = next_pos;

                // desplazar la ruta
                for (int i = 1; i < heroes[id].ruta_length; i++)
                {
                    heroes[id].ruta[i - 1] = heroes[id].ruta[i];
                }
                heroes[id].ruta_length--;
                // printf("Héroe se mueve a (%d, %d)\n", next_pos.x, next_pos.y);
            }
            else if (heroes[id].ruta_length == 0)
            {
                printf("Llegó a su destino\n");
                exit(0);
            }
            else
            {
                printf("Héroe no tiene ruta para moverse\n");
            }
        }

        pthread_mutex_unlock(&mutex);

        sem_post(&heroe_done[id]);
    }
    return NULL;
}

void *monstruo(void *arg)
{
    int id = *(int *)arg;

    while (true)
    {
        sem_wait(&turno_monstruos[id]);

        pthread_mutex_lock(&mutex);

        // Lógica del monstruo
        if (monstruos[id].vida > 0)
        {
            // printf("Monstruo %d actuando\n", id);

            int rango_vision = *monstruos[id].rango_vision ? *monstruos[id].rango_vision : 0;

            // calcula distancia con el heroe mas cercano
            Coord monstruo_pos = monstruos[id].current_coords;
            int min_distancia = width + height; // distancia máxima posible
            int closest_heroe_id = -1;
            for (int i = 0; i < cantidad_heroes; i++)
            {
                if (heroes[i].vida > 0)
                {
                    Coord heroe_pos = heroes[i].current_coords;
                    int distancia = distanciaManhattan(monstruo_pos.x, monstruo_pos.y, heroe_pos.x, heroe_pos.y);
                    if (distancia < min_distancia && heroes[i].estado[0] != 'f')
                    {
                        min_distancia = distancia;
                        closest_heroe_id = i;
                    }
                }
            }

            if (closest_heroe_id == -1)
            {
                // No hay héroes disponibles
                pthread_mutex_unlock(&mutex);
                sem_post(&monstruo_done[id]);
                continue; // Salta este turno
            }

            Coord heroe_pos = heroes[closest_heroe_id].current_coords;
            int distancia = distanciaManhattan(monstruo_pos.x, monstruo_pos.y, heroe_pos.x, heroe_pos.y);

            if (monstruos[id].estado[0] == 'a') // 'a' de activo
            {
                int rango_ataque = monstruos[id].rango_ataque ? monstruos[id].rango_ataque : 0;
                if (distancia <= rango_ataque)
                {
                    // le pega al heroe
                    heroes[closest_heroe_id].vida -= monstruos[id].daño;
                    printf("Monstruo %d ataca al héroe! Vida restante del héroe: %d\n", id, heroes[closest_heroe_id].vida);

                    if (heroes[closest_heroe_id].vida <= 0)
                    {
                        printf("El héroe %d ha sido derrotado! Fin del juego.\n", closest_heroe_id);
                    }
                }
                else
                {
                    //printf("Monstruo %d está fuera de rango de ataque. Avanzando...\n", id);

                    // Calcular distancia y generar nueva ruta
                    int distancia_hacia_heroe = distanciaManhattan(
                        monstruos[id].current_coords.x,
                        monstruos[id].current_coords.y,
                        heroe_pos.x,
                        heroe_pos.y);

                    // Reservar memoria para la nueva ruta
                    Coord *nueva_ruta = malloc((distancia_hacia_heroe + 1) * sizeof(Coord));
                    if (nueva_ruta == NULL)
                    {
                        printf("Error: No se pudo asignar memoria para la ruta\n");
                        break;
                    }

                    int nueva_ruta_length = generarCamino(
                        monstruos[id].current_coords.x,
                        monstruos[id].current_coords.y,
                        heroe_pos.x,
                        heroe_pos.y,
                        nueva_ruta);

                    // IMPORTANTE: Liberar la ruta anterior antes de asignar la nueva
                    if (monstruos[id].ruta != NULL)
                    {
                        free(monstruos[id].ruta);
                    }

                    // Actualizar la ruta del monstruo
                    monstruos[id].ruta = nueva_ruta; // Ahora sí, asignar directamente
                    monstruos[id].ruta_length = nueva_ruta_length;

                    // Verificar que hay al menos un paso en la ruta
                    if (monstruos[id].ruta_length <= 0)
                    {
                        printf("Error: Ruta vacía para monstruo %d\n", id);
                        break;
                    }

                    // Obtener siguiente posición (índice 1, no 0, porque 0 es la posición actual)
                    Coord next_pos;
                    if (monstruos[id].ruta_length > 1)
                    {
                        next_pos = monstruos[id].ruta[1]; // El siguiente paso
                    }
                    else
                    {
                        // Ya está en el destino
                        next_pos = monstruos[id].ruta[0];
                    }

                    // Actualizar matriz
                    mover_entidad_matriz(
                        matriz,
                        monstruos[id].current_coords.x,
                        monstruos[id].current_coords.y,
                        next_pos.x,
                        next_pos.y,
                        width,
                        height);

                    // Actualizar coordenadas actuales
                    monstruos[id].current_coords = next_pos;

                    // Desplazar la ruta (eliminar el primer elemento)
                    for (int i = 0; i < monstruos[id].ruta_length - 1; i++)
                    {
                        monstruos[id].ruta[i] = monstruos[id].ruta[i + 1];
                    }
                    monstruos[id].ruta_length--;
                }
            }

            if (distancia <= rango_vision)
            {
                printf("Monstruo %d ve al héroe a distancia %d\n", id, distancia);

                // si esta en rango, despierta a todos los monstruos dentro de su rango de vision
                for (int j = 0; j < cantidad_monstruos; j++)
                {
                    if (j != id && monstruos[j].vida > 0)
                    {
                        Coord otro_monstruo_pos = monstruos[j].current_coords;
                        int distancia_entre_monstruos = distanciaManhattan(monstruo_pos.x, monstruo_pos.y, otro_monstruo_pos.x, otro_monstruo_pos.y);
                        if (distancia_entre_monstruos <= rango_vision)
                        {
                            printf("Monstruo %d despierta al monstruo %d\n", id, j);

                            monstruos[j].estado[0] = 'a'; // 'a' de activo
                        }
                    }
                }

                // si esta activo, ve si tiene el rango de ataque suficiente para pegarle al heroe

                if (monstruos[id].estado[0] != 'a')
                {
                    // si no esta activo, se activa y no hace nada
                    monstruos[id].estado[0] = 'a'; // 'a' de activo
                }
            }

            usleep(10000); // Simular tiempo de acción
        }

        pthread_mutex_unlock(&mutex);

        sem_post(&monstruo_done[id]);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }

    matriz = read_game_config(argv[1], &width, &height, &total_entidades, 
                              &heroes, &monstruos, &cantidad_monstruos, &cantidad_heroes);

    if (!matriz)
    {
        printf("Error al leer la configuración\n");
        return 1;
    }

    inicializar_estructuras_dinamicas();

    // Imprimir estado inicial
    printf("Estado inicial del juego:\n");
    printf("Héroes: %d, Monstruos: %d\n", cantidad_heroes, cantidad_monstruos);
    print_matriz_simple(matriz, width, height, cantidad_heroes);

    pthread_mutex_init(&mutex, NULL);

    // Arrays de threads e IDs
    pthread_t agente_thread;
    pthread_t heroe_threads[cantidad_heroes];
    int heroe_ids[cantidad_heroes];
    
    pthread_t monstruo_threads[cantidad_monstruos];
    int monstruo_ids[cantidad_monstruos];

    // Crear thread del agente
    pthread_create(&agente_thread, NULL, agente, NULL);
    for (int i = 0; i < cantidad_heroes; i++)
    {
        heroe_ids[i] = i;
        pthread_create(&heroe_threads[i], NULL, heroe, &heroe_ids[i]);
    }

    // Crear threads de monstruos correctamente
    for (int i = 0; i < cantidad_monstruos; i++)
    {
        monstruo_ids[i] = i;
        pthread_create(&monstruo_threads[i], NULL, monstruo, &monstruo_ids[i]);
    }

    // Join threads
    pthread_join(agente_thread, NULL);
    
    for (int i = 0; i < cantidad_heroes; i++)
    {
        pthread_join(heroe_threads[i], NULL);
    }

    for (int i = 0; i < cantidad_monstruos; i++)
    {
        pthread_join(monstruo_threads[i], NULL);
    }

    // Limpiar recursos
    for (int i = 0; i < cantidad_heroes; i++)
    {
        sem_destroy(&turno_heroe[i]);
        sem_destroy(&heroe_done[i]);
    }

    for (int i = 0; i < cantidad_monstruos; i++)
    {
        sem_destroy(&turno_monstruos[i]);
        sem_destroy(&monstruo_done[i]);
    }

    pthread_mutex_destroy(&mutex);
    free_matriz(matriz, height, width);

    return 0;
}