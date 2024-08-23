// VPACKAGE MANAGER
// Made by bradinator

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

void vpack_install(void);

void vpack_update(void);

void vpack_remove(void);

void vpack_search(void);

void vpack_list(void);

typedef struct {
    char name[100];
    char version[10];
    char description[256];
    char author[50]; 
    uint32_t id;
} Package;

#endif
