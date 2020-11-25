#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// fonction utilisé seulement dans la fonction saisieClavier
void viderBuffer(){
    int c = 0;
    while (c != '\n' && c != EOF){
        c = getchar();
    }
}
 
int saisieClavier(char *chaine, int longueur){
    char *positionEntree = NULL;
    if (fgets(chaine, longueur, stdin) != NULL){
        positionEntree = strchr(chaine, '\n');
        if (positionEntree != NULL){
            *positionEntree = '\0';
        }else{
            viderBuffer();
        }
        return 1;
    }else{
        viderBuffer();
        return 0;
    }
}

// etatSystemeLocal est la copie de l'état du système du server, 
void afficherEtatSysteme(SystemState_s *etatSystemeLocal){
    printf("\nEtat du système :\n");
    for(int i=0;i<etatSystemeLocal->nbSites;i++){
        printf("  - [Site %d] : %d cpu free, %.1f Go free \n",   
                etatSystemeLocal->sites[i].id, 
                etatSystemeLocal->sites[i].cpu,
                etatSystemeLocal->sites[i].sto);
    }
}

void afficherStructureRequete(Modification_m *m){
    if(m->type == 1){
        printf("\n[Demande de ressources]");
    }else{
        printf("\n[Demande de libération de ressources]");
    }

    printf("\n En mode exclusif : ");
    if(m->nbExclusiveMode == 0){
        printf("\n   [Aucune]");
    }else{
        
        for(int i=0;i<m->nbExclusiveMode;i++){
            printf("\n   [Site %d] : %d cpu, %.1f Go",   
                m->exclusiveMode[i].idSite, 
                m->exclusiveMode[i].cpu,
                m->exclusiveMode[i].sto);
        }
    }

    printf("\n En mode partagé : ");
    if(m->nbShareMode == 0){
        printf("\n   [Aucune]");
    }else{
        for(int i=0;i<m->nbShareMode;i++){
            printf("\n   [Site %d] : %d cpu free, %.1f Go free \n",   
                m->shareMode[i].idSite, 
                m->shareMode[i].cpu,
                m->shareMode[i].sto);
        }
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
        modification.nbExclusiveMode = 0;
        modification.nbShareMode = 0;
        // Demande à l'utilisateur ce qu'il veut

        char bufAction[2]; // Buffer qui contiendra la demande de l'action (demande ou libération)
        modification.type = 0;
        printf("\nVoici les actions possible :\n");
        printf("  1 : demande de ressources\n");
        printf("  2 : libération de ressources\n");
        do{
            printf("\nQuelle action voulez-vous effectuer ? (1 ou 2) : ");
            saisieClavier(bufAction, 2); // récupère la saisie de l'utilisateur dans le buffer
            modification.type = atoi(bufAction); // transforme la chaine de char en int
        }while(modification.type  != 1 && modification.type  != 2 );



        if(modification.type == 1){ // Pour une demande de ressource

            int inputId, inputMode, inputCpu;
            float inputSto;
            int response =0; // booleen pour savoir si on refait une autre demande de ressource
            do{
                printf("\nLa demande doit correspondre à ce format : idSite mode nbCPU nbSto\n");
                printf("Demande de ressource : ");
                if(scanf("%d %d %d %f",&inputId, &inputMode, &inputCpu, &inputSto) != 4 ){ // Pour vérifier le format
                    // Si le format n'est pas correct on indique la bonne utilisation à l'utilisateur on on quitte le programme
                    printf("\nErreur : La demande doit correspondre à ce format : idSite mode nbCPU nbSto\n");
                        printf("\t idSite : l'identifiant du site \n");
                        printf("\t mode   : 1 pour le mode exclusif, 2 pour le mode partagé \n");
                        printf("\t nbCPU  : Le nombre de CPU demandé \n");
                        printf("\t nbSto  : Le nombre de Go de stockage demandé \n");
                    exit(1); 
                    
                }else{ // le format est correct mais cela ne veut pas dire que la demande l'est.
                    printf("[Demande de ressource] Site %d : %d cpu, %.1f Go en mode %d\n", inputId, inputCpu, inputSto, inputMode);
                      
                    /* faire une vérification de la demande (pour voir si elle est possible à repondre, 
                    par exemple si le site 1 ne possède que 200cpu à l'initialisation du site, 
                    un utilisateur ne pas pas en demandé 300, c'est juste pas possible)*/

                    // après vérification (si erreur : annulé la demande)


                        //il faut vérifier que l'identifiant du site est correct
                        // if ...

                        // il faut vérifier si la demande de ce nombre de cpu et de sto pour ce site est possible
                        // if ...
                        if(0){
                            perror("Erreur : Le nombre de ressource demandé est supérieur au nombre de ressource initialement disponible sur le site\n");
                            exit(1); 
                        }
                    // Sinon on entre la demande
                    if(inputMode == 1){
                        modification.nbExclusiveMode++;
                        modification.exclusiveMode[modification.nbExclusiveMode-1].idSite = inputId;
                        modification.exclusiveMode[modification.nbExclusiveMode-1].cpu = inputCpu;
                        modification.exclusiveMode[modification.nbExclusiveMode-1].sto = inputSto;
                    
                    }else if(inputMode == 2){
                        modification.nbShareMode++;
                        modification.shareMode[modification.nbShareMode-1].idSite = inputId;
                        modification.shareMode[modification.nbShareMode-1].cpu = inputCpu;
                        modification.shareMode[modification.nbShareMode-1].sto = inputSto;
                    }else{
                        perror("erreur : le mode doit être 1 pour <mode exclusif> ou 2 pour <mode partagé>");
                        exit(1);
                    }
                    

                    printf("\nEntre 1 si tu veux faire une autre demande (sinon met autre chose) : ");
                    scanf("%d",&response);
                    if(response != 1 ){ 
                        response = 0;
                    }
                }
            }while(response);

        }else{ // donc on est obligatoirement dans le cas d'une libération (on aurait pas pu arrivé ici sinon)
            // on regarde si il y a des ressources à libérer
            // si c'est le cas alors on spécifie combien de ressources on veut libérer
                // vérifie si la demande est correct
                // si c'est incorrect on annule la demande
                // sinon on effectue la demande de libération du nombre de ressources demandé
        }

        afficherStructureRequete(&modification);
    //}

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt -> détachement segment mémoire : état du système");
        exit(1);
    }

    printf("\nFin du Programme \n");

    return 0;
}