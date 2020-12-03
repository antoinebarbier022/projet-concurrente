#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

void afficherInitEtatSysteme(SystemState_s *s){
    printf("\033[36m "); // couleur du texte
    printf("\nEtat du système : (%d sites)\n", s->nbSites);
    for(int i=0;i<s->nbSites;i++){
        printf("  - %-15s[id=%d] : %-4d cpu, %-4d Go, nbResExclusif : %-4d, nbResShare : %-4d \n",   
                s->sites[i].nom, 
                s->sites[i].id, 
                s->sites[i].cpu ,
                s->sites[i].sto ,
                s->sites[i].nbMaxClientEx,
                s->sites[i].nbMaxClientSh);
    }
    printf("\033[m\n");
}

int main(int argc, char const *argv[])
{

    // récuperer les identifiants des ipc
    key_t cle = ftok(argv[1], 'e');

    // identification du segment mémoire pour l'état su système
    int shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666);
    if(shm_id == -1) {
        perror("erreur : shmget -> identification segment mémoire pour l'état du système");
        exit(1);
    }

    //attachement au segement memoire
    struct SystemState_s* p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat -> lors de l'attachement au segment mémoire");
        exit(1);
    }
    afficherInitEtatSysteme(p_att);

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("Erreur : shmdt -> lors du détachement du segment mémoire.");
        exit(1);
    }

    return 0;
}
