#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>

// etatSystemeLocal est la copy de l'état du système du server, 
void afficherEtatSysteme(SystemState_s *etatSystemeLocal){
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

    // récuperer les identifiants des ipc
    key_t cle = ftok(argv[1], 'r');         // clé du segment mémoire pour l'état du système
    key_t cle_sem = ftok(argv[1], 'u');     // clé du tableau de sémaphore

    // identification du segment mémoire
    int shm_id = shmget(cle, sizeof(struct SystemState_s), 0666);
    if(shm_id == -1) {
        perror("erreur : shmget -> identification segment mémoire : état du système");
        exit(1);
    }

    // identification d'un tableau de 2 sémaphore
    int sem_id = semget(cle_sem, 2, IPC_CREAT|0666);
    if(sem_id == -1) {
        perror("erreur : semget tableau de 2 semaphores");
        exit(1);
    }

    //initialisation des opérations pour le sémaphore qui sert de lock (num 0) 
    struct sembuf opLock[]={
        {(u_short)0,(short)-1,SEM_UNDO},
        {(u_short)0,(short)+1,SEM_UNDO}
    };

    //attachement au segement memoire
    struct SystemState_s* p_att = shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat -> attachemnt segment mémoire : état du système");
        exit(1);
    }

    //Etape 1 : créer une copie local du système

    // copie de l'état système sur le client
    printf("Attente de la copie du système\n");
    // ici -> mettre un lock car on lit le segment mémoire
    semop(sem_id,opLock,1); // P sur le semaphore qui sert de lock pour l'état du système
    struct SystemState_s etatSystemCopyOnClient = *p_att;
    printf("... Copy en cours ...\n");
    sleep(1);
    // ici -> mettre un unlock 
    semop(sem_id,opLock+1,1); // P sur le semaphore qui sert de lock pour l'état du système
    printf("La copy est terminé\n");

    //Etape 2 : creation du processus secondaire (thread) 
    //          qui se charge d'afficher l'état du système à chaque mise à jour

        // il faut la mettre à jour avec les modifications reçut
    /*- on affiche l'état seulement quand il y a une modification
      - detecté modification du sytème
      - mettre à jour la copie 
      - afficher l'état du système mis à jour */
    afficherEtatSysteme(&etatSystemCopyOnClient);

    //Etape 3 :  Boucle qui attent les demandes de modifications (demande/libération)
    //while(1){
        struct Modification_m modification;
        // Demande à l'utilisateur ce qu'il veux

        char bufAction[1]; // Buffer de la demande de l'action
        modification.type = 0;
        printf("\nVoici les actions possible :\n");
        printf("  - demande de ressources : 1\n");
        printf("  - libération de ressources : 2\n");
        do{
            printf("Quelle action voulez-vous effectuer ? (1 ou 2) : ");
            gets(bufAction); // Récupération de la saisie de l'utilisateur
            modification.type = atoi(bufAction); // on convertie le char en int
        }while(modification.type  != 1 && modification.type  != 2 );

        printf("La reponse est : %d\n",modification.type);
        
        if(modification.type == 1){
            //

        }else{ // donc on est obligatoirement dans le cas d'une libération
            // on regarde si il y a des ressources à libérer
            // si c'est le cas alors on spécifie combien de ressources on veut libérer
                // vérifie si la demande est correct
                // si c'est incorrect on annule la demande
                // sinon on effectue la demande de libération du nombre de ressources demandé
        }
        

    //}

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt -> détachement segment mémoire : état du système");
        exit(1);
    }

    return 0;
}