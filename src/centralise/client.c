#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>

// etatSystemeLocal est la copy de l'état du système du server, 
//    il faut la mettre à jour avec les modifications reçut
//    
void afficherEtatSysteme(SystemState_s *etatSystemeLocal){
    // on affiche l'état seulemnt quand il y a une modification
    // detecté modification du sytème
    // mettre à jour la copie 
    // afficher l'état du système mis à jour

    printf("\nEtat du système :\n");
    for(int i=0;i<etatSystemeLocal->nbSites;i++){
        printf("  - [Site %d] : %d cpu free, %.1f Go free \n",   
                etatSystemeLocal->sites[i].id, 
                etatSystemeLocal->sites[i].cpu,
                etatSystemeLocal->sites[i].sto);
    }

}

int main(int argc, char const *argv[]){

    if(argc != 2){
        printf("lancement : ./server chemin_fichier_cle\n");
        exit(1);
    }

    // récuperer l'identifiant du segement mémoire qu'on souhaite utiliser.
    key_t cle = ftok(argv[1], 'r');

    // identification du segment mémoire
    int shm_id = shmget(cle, sizeof(struct SystemState_s), 0666);
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

    // copy de l'état système sur le client

    // ici -> mettre un lock car on lit le segment mémoire
    struct SystemState_s etatSystemCopyOnClient = *p_att;
    // ici -> mettre un unlock 


    afficherEtatSysteme(&etatSystemCopyOnClient);
    


    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt");
        exit(1);
    }

    return 0;
}