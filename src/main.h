#ifndef MAIN_H
#define MAIN_H

#include "system.h"
#include "cpu.h"
#include "memory.h"
#include "loader.h"
#include "dma.h"
#include "disk.h"
#include "logger.h"

void init();
void start_process(const char* prog_name);
void shell();
int main();

#endif