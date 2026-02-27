#ifndef LOADER_H
#define LOADER_H

#include "system.h"

void clean_line(char *line);
int load_program(const char *filename, int base_address);

#endif