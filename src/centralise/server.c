#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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

int main(int argc, char const *argv[]){


    if(argc != 2){
        printf("lancement : ./server chemin_fichier_cle\n");
        exit(1);
    }


    // récuperer l'identifiant du segement mémoire qu'on souhaite utiliser.
    key_t cle = ftok(argv[1], 'r'); 

    // identification du segment mémoire
    int shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666);
    if(shm_id == -1) {
        perror("erreur : shmget");
        exit(1);
    }

    //attachement au segement memoire
    struct SystemState_s* p_att = shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat");
        exit(1);
    }

    // initialisation du segment mémoire avec l'état du système

    // initialisation des sites
    
    Site_s lyon = {1, 128, 2000};
    Site_s montpellier = {2, 64, 1400};
    Site_s Toulouse = {3, 128, 1400};
    
    // initialisation de l'état du système avec les sites à l'intérieur

    // ici -> mettre un lock car on ecrit le segment mémoire
    p_att->nbSites = 3;
    p_att->sites[0] = lyon;
    p_att->sites[1] = montpellier;
    p_att->sites[2] = Toulouse;
    
    printf("Exemple : le site de Lyon possède %d cpu \n", p_att->sites[0].cpu);
    printf("Exemple : le site de Montpellier possède %d cpu \n", p_att->sites[1].cpu);
    printf("Exemple : le site de Toulouse possède %d cpu \n", p_att->sites[2].cpu);
    // ici -> mettre un unlock 


    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt");
        exit(1);
    }

    // destruction de l'objet IPC
    //shmctl(shm_id,0,IPC_RMID);

    return 0;
}
