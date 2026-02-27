#include "loader.h"

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

int load_program(const char *filename, int base_address) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Archivo no Encontrado\n");
        return -1;
    }

    char line[256];
    int current_addr = base_address;
    int instructions_loaded = 0;

    int prev_mode = sys.cpu_registers.PSW.operation_mode;
    sys.cpu_registers.PSW.operation_mode = 1; 

    while (fgets(line, sizeof(line), file)) {
        clean_line(line);
        if (strlen(line) == 0) continue;
        //Procesar Directivas
        if (line[0] == '.') {
            if (strncmp(line, ".NombreProg", 11) == 0) {
                write_in_log(line);
            }
            continue; 
        }
        //Procesar Instrucciones
        char *endptr;
        long instruction = strtol(line, &endptr, 10); 
        if (line != endptr) {
            if (current_addr >= MEM_SIZE) {
                break;
            }
            memory_write(current_addr, (int)instruction);
            current_addr++;
            instructions_loaded++;
        }
    }
    memory_write(current_addr, 99000000);
    fclose(file);
    sys.cpu_registers.PSW.operation_mode = prev_mode;
    return instructions_loaded;
}