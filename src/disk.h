#ifndef DISK_H
#define DISK_H

#include "system.h"

#define NUM_TRACKS 10
#define NUM_CYLINDERS 10
#define NUM_SECTORS 100
#define TAM_SECTOR 9

//structs

typedef struct {
    unsigned char data[TAM_SECTOR];
} Sector;

typedef struct {
    Sector platter[NUM_TRACKS][NUM_CYLINDERS][NUM_SECTORS];
} Disk;

//funciones
void init_disk(void);

//t = track, c = cylinder, s = sector
void read_sector(int t, int c, int s, char *destiny);

void write_sector(int t, int c, int s, char *origin);

#endif
