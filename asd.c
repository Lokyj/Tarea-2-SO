#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "entidades.h"
#include "parser.h"
#include "matriz_hooks.h"

#define NUM_MONSTRUOS 5

// tablero
Entidad*** matriz;
int width, height, total_entidades;

// Semáforos para controlar turnos
sem_t turno_heroe;
sem_t turno_monstruos[NUM_MONSTRUOS];

// Semáforos para señalar finalización
sem_t heroe_done;
sem_t monstruo_done[NUM_MONSTRUOS];

pthread_mutex_t mutex;

void *agente(void *arg) {
    while (true) {
        // Turno del héroe
        sem_post(&turno_heroe);
        sem_wait(&heroe_done);
        
        // Turno de cada monstruo
        for (int i = 0; i < NUM_MONSTRUOS; i++) {
            sem_post(&turno_monstruos[i]);
            sem_wait(&monstruo_done[i]);
        }
        
        printf("--- Ciclo completado ---\n");
        print_matriz_simple(matriz, width, height);
    }
    return NULL;
}

void *heroe(void *arg) {
    while (true) {
        sem_wait(&turno_heroe);
        
        pthread_mutex_lock(&mutex);
        // Lógica del héroe
        printf("Héroe actuando\n");
        pthread_mutex_unlock(&mutex);
        
        sem_post(&heroe_done);
    }
    return NULL;
}

void *monstruo(void *arg) {
    int id = *(int*)arg;
    
    while (true) {
        sem_wait(&turno_monstruos[id]);
        
        pthread_mutex_lock(&mutex);
        // Lógica del monstruo
        printf("Monstruo %d actuando\n", id);
        sleep(rand() % 1 + 1); // Simular tiempo de acción

        pthread_mutex_unlock(&mutex);
        
        sem_post(&monstruo_done[id]);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }
    
    matriz = read_game_config(argv[1], &width, &height, &total_entidades);
    
    if (!matriz) {
        printf("Error al leer la configuración\n");
        return 1;
    }
    
    // Imprimir estado inicial
    printf("Estado inicial del juego:\n");
    print_matriz_simple(matriz, width, height);
    
    pthread_t agente_thread;
    pthread_t heroe_thread;
    pthread_t monstruo_threads[NUM_MONSTRUOS];
    int monstruo_ids[NUM_MONSTRUOS];
    
    // Inicializar semáforos
    sem_init(&turno_heroe, 0, 0);
    sem_init(&heroe_done, 0, 0);
    
    for (int i = 0; i < NUM_MONSTRUOS; i++) {
        sem_init(&turno_monstruos[i], 0, 0);
        sem_init(&monstruo_done[i], 0, 0);
        monstruo_ids[i] = i;
    }
    
    pthread_mutex_init(&mutex, NULL);
    
    // Crear threads
    pthread_create(&agente_thread, NULL, agente, NULL);
    pthread_create(&heroe_thread, NULL, heroe, NULL);
    
    for (int i = 0; i < NUM_MONSTRUOS; i++) {
        pthread_create(&monstruo_threads[i], NULL, monstruo, &monstruo_ids[i]);
    }
    
    // Join threads (nunca se alcanza en este caso)
    pthread_join(agente_thread, NULL);
    pthread_join(heroe_thread, NULL);
    
    for (int i = 0; i < NUM_MONSTRUOS; i++) {
        pthread_join(monstruo_threads[i], NULL);
    }
    
    // Limpiar recursos
    sem_destroy(&turno_heroe);
    sem_destroy(&heroe_done);
    
    for (int i = 0; i < NUM_MONSTRUOS; i++) {
        sem_destroy(&turno_monstruos[i]);
        sem_destroy(&monstruo_done[i]);
    }
    
    pthread_mutex_destroy(&mutex);
    free_matriz(matriz, height, width);
    
    return 0;
}