#include "struct.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct SystemState_s SystemState_s;
typedef struct Site_s Site_s;
typedef struct Use_s Use_s;

/* 
initialisation de l'état du système : 
On définie 3 sites : 
 - Lyon (id : 1, cpu : 128, sto : 2000), 
 - Montpellier (id : 2, cpu : 64, sto : 1400),
 - Toulouse (id : 2, cpu : 64, sto : 1400)
*/



void initEtatSysteme(SystemState_s* s){
    // initialisation des sites
    Site_s lyon = {1, 128, 2000};
    Site_s montpellier = {2, 64, 1400};
    Site_s Toulouse = {3, 128, 1400};

    // initialisation de l'état du système avec les sites à l'intérieur
    s->sites = malloc(3*sizeof(Site_s)); // on alloue de la mémoire pour mettre 3 sites
    s->sites[0] = lyon;
    s->sites[1] = montpellier;
    s->sites[2] = Toulouse;
    printf("[Structure initialisé pour l'état du système] \n");
}



int main(int argc, char const *argv[]){
    SystemState_s etatSystem;
    initEtatSysteme(&etatSystem);
    printf("Exemple : le site de Montpellier possède %d cpu \n", etatSystem.sites[1].cpu);
    return 0;
}
