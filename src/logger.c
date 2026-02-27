#include "loader.h"

// Función genérica para registrar cualquier evento
void write_in_log(const char *event) {
    //Toma control del Semaforo
    pthread_mutex_lock(&sys.log_mutex);

    FILE *archive = fopen("resume.log", "a");
    if (archive == NULL) return;
    fprintf(archive, "%s\n", event);
    fclose(archive);

    //Libera el Semaforo
    pthread_mutex_unlock(&sys.log_mutex);
};

//Funcion para Limpiar el Log
void clean_log(){
    //Toma control del Semaforo
    pthread_mutex_lock(&sys.log_mutex);
    FILE *archive = fopen("resume.log", "w");
    if (archive == NULL) return;
    fprintf(archive, "\n");
    fclose(archive);
    //Libera el Semaforo
    pthread_mutex_unlock(&sys.log_mutex);
};