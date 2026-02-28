#include "loader.h"
#include "disk.h"

//Control de Disco
DISK_DIR disk_reg[MULTIPROGRAMING_GRADE];
int prog_count = 0;
int next_track = 0;
int next_cylinder = 0;

// Función auxiliar para limpiar la línea 
void clean_line(char *line) {
    char *comment = strstr(line, "//"); // Buscar comentarios
    if (comment) {
        *comment = '\0'; // Cortar el string donde empieza el comentario
    }
    // Quitar salto de línea final si existe
    line[strcspn(line, "\n")] = 0;
    line[strcspn(line, "\r")] = 0;
}

int load_program(const char *filename, const char *prog_name) {
    if(prog_count >= MULTIPROGRAMING_GRADE){
        printf("Error: Disco Lleno");
        return -1;
    }
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Archivo no Encontrado\n");
        return -1;
    }

    char line[256];
    int instructions_loaded = 0;
    char sector_buffer[TAM_SECTOR];
    int current_sector = 0;
    //Registrar el programa
    strcpy(disk_reg[prog_count].prog_name, prog_name);
    disk_reg[prog_count].track = next_track;
    disk_reg[prog_count].cylinder = next_cylinder;
    disk_reg[prog_count].sector = 0; 
    //Leer archivo y guardar cada instrucción en un sector distinto
    while (fgets(line, sizeof(line), file)) {
        clean_line(line);
        if (strlen(line) == 0) continue;
        if (line[0] == '.') {
            if (strncmp(line, ".NombreProg", 11) == 0) {
                write_in_log(line);
            }
            continue; 
        }
        char *endptr;
        long instruction = strtol(line, &endptr, 10); 
        if (line != endptr) {
            memset(sector_buffer, 0, TAM_SECTOR);
            //Guardamos la instrucción como TEXTO para que tu DMA pueda usar atoi()
            sprintf(sector_buffer, "%ld", instruction); 
            write_sector(next_track, next_cylinder, current_sector, sector_buffer);
            current_sector++;
            instructions_loaded++;
        }
    }
    //Escribir el HALT de seguridad al final en su propio sector
    memset(sector_buffer, 0, TAM_SECTOR);
    sprintf(sector_buffer, "99000000");
    write_sector(next_track, next_cylinder, current_sector, sector_buffer);
    instructions_loaded++;
    fclose(file);
    //Actualizar el catálogo
    disk_reg[prog_count].size = instructions_loaded;
    prog_count++;
    //Avanzamos al siguiente cilindro para el próximo programa
    next_cylinder++; 
    char log_msg[256];
    sprintf(log_msg, "KERNEL >> Programa '%s' cargado en Disco (Cilindro %d).", prog_name, next_cylinder - 1);
    write_in_log(log_msg);
    return instructions_loaded;
}