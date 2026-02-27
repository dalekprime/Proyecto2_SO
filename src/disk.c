#include "disk.h"

// Variable privada del archivo (Hardware encapsulado)
static Disk fisic_disk;

// Inicializa el disco llenándolo de ceros
void init_disk(void) {
    memset(&fisic_disk, 0, sizeof(Disk));
}

// Lectura: Hardware -> Ram (destiny)
void read_sector(int t, int c, int s, char *destiny) {
    //validacion para no salirse de los limites
    if (t < 0 || t >= NUM_TRACKS ||
        c < 0 || c >= NUM_CYLINDERS ||
        s < 0 || s >= NUM_SECTORS) {
        return;
    }

    memcpy(destiny, fisic_disk.platter[t][c][s].data, TAM_SECTOR);

    return;
}

// Escritura: Ram (origin) -> Hardware
void write_sector(int t, int c, int s, char *origin) {

    if (t < 0 || t >= NUM_TRACKS ||
        c < 0 || c >= NUM_CYLINDERS ||
        s < 0 || s >= NUM_SECTORS) {
        return;
    }

    memcpy(fisic_disk.platter[t][c][s].data, origin, TAM_SECTOR);

    return;
}
