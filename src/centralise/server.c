#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#define NBSITES 10 
#define NBMAXCLIENT 20

typedef struct SystemState_s SystemState_s;
typedef struct Site_s Site_s;
typedef struct Use_s Use_s;

using namespace std;

int main(int argc, char const *argv[]){


    if(argc != 2){
        printf("lancement : ./server chemin_fichier_cle\n");
        exit(1);
    }


    // récuperer les identifiants des ipc
    key_t cle = ftok(argv[1], 'z');         // clé du segment mémoire pour l'état du système
    key_t cle_sem = ftok(argv[1], 'u');     // clé du tableau de sémaphore

    // identification du segment mémoire pour l'état su système
    int shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666);
    if(shm_id == -1) {
        perror("erreur : shmget -> identification segment mémoire pour l'état du système");
        exit(1);
    }

    // identification d'un tableau de 2 sémaphores
    // Le semaphore numero 1 sert de lock pour accédé au segment mémoire de l'état du système
    // Le semaphore numero 2 sert pour la notification de la modification de l'état mémoire
    int sem_id = semget(cle_sem, 2, IPC_CREAT|0666);
    if(sem_id == -1) {
        perror("erreur : semget tableau de 2 semaphores");
        exit(1);
    }

    union semun{
        int val;                // cmd= SETVAL
        struct semid_ds *buf;   // cmd= IPC_STAT ou IPC_SET
        unsigned short *array;  // cmd= GETALL ou SETALL
        struct seminfo *__buf;  // cmd= IPC_INFO (sous linux)
    }egCtrl;

    // initialisation des semaphores
    egCtrl.val = 1; // valeur du semaphore
    if(semctl(sem_id, 0, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 1
        perror("erreur : init du semaphore 0");
        exit(1);
    }

    /* On met la valeur de ce sémaphore = au nombre de client afin de s'assurer 
    que tous les clients on lu la modification apporté au système avant de continuer leurs actions 
    On devra mettre en place un systeme de rdv pour attentre que tous les clients ai fini de mettre à jour 
    leurs copy local du système avant de supprimer le contenu du segment mémoire pour les modifications*/
    int nbClientConecte = 1;
    egCtrl.val = nbClientConecte; 
    if(semctl(sem_id, 1, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 2
        perror("erreur : init du semaphore 1");
        exit(1);
    }

    //initialisation des opérations pour le sémaphore qui sert de lock (num 0) 
    // Les numéros de sémaphores commencent à 0
    /*La structure de sembuf est la suivante :
    struct sembuf{
        unsigned short sem_num; // numéro du semaphore
                 short sem_op;  // Opération sur le semaphore
                 short sem_flg; // Options par exemple SEM_UNDU
    };
    La structure de sembuf est déja défini dans la bibliothèque*/
    // On initialise sembuf de cet façon, mais on peut le faire autrement
    struct sembuf opLock[]={
        {(u_short)0,(short)-1,SEM_UNDO},
        {(u_short)0,(short)+1,SEM_UNDO}
    };

    //initialisation des opérations pour le sémaphore aux modifications (num 1) 
    /*struct sembuf opMod[]={
        {(u_short)1,(short)-1,SEM_UNDO},
        {(u_short)1,(short)+1,SEM_UNDO}
    };*/

    //attachement au segement memoire
    struct SystemState_s* p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat -> attachement segment mémoire état du système");
        exit(1);
    }

    // initialisation du segment mémoire avec l'état du système

    // initialisation des sites
    /* On définie 3 sites : 
    - Lyon (id : 1, cpu : 128, sto : 2000), 
    - Montpellier (id : 2, cpu : 64, sto : 1400),
    - Toulouse (id : 2, cpu : 64, sto : 1400)
    */
    Site_s lyon = {1, 128, 2000, 128, 2000, 0, 0};
    Site_s montpellier = {2, 64, 1400, 64, 1400, 0, 0};
    Site_s Toulouse = {3, 128, 1400, 128, 1400, 0, 0};
    
    // initialisation de l'état du système à l'intérieur du segment mémoire

    
    // ici -> mettre un lock car on ecrit le segment mémoire
    semop(sem_id,opLock,1); // P sur le semaphore qui sert de lock pour l'état du système

    p_att->nbSites = 3;
    p_att->sites[0] = lyon;
    p_att->sites[1] = montpellier;
    p_att->sites[2] = Toulouse;
    for(int indexSite=0;indexSite<p_att->nbSites;indexSite++){
        for(int i=0;i<NBMAXCLIENT;i++){
            p_att->sites[indexSite].tabUse[i].isUse = 0; // on init le boolean à 0 car il n'y a pas encore d'utilisation au lancement du server
        }
    }

    
    printf("Exemple : le site de Lyon possède %d cpu \n", p_att->sites[0].cpu);
    printf("Exemple : le site de Montpellier possède %d cpu \n", p_att->sites[1].cpu);
    printf("Exemple : le site de Toulouse possède %d cpu \n", p_att->sites[2].cpu);
    // ici -> mettre un unlock 
    semop(sem_id,opLock+1,1); // V sur le semaphore qui sert de lock pour l'état du système


    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt -> détachement segment mémoire état du système");
        exit(1);
    }

    // destruction de l'objet IPC
    int touche;
    cout << "Appuyer sur une touche pour arreté le server ..." << endl;
    cin >> touche;
    
    shmctl(shm_id,0,IPC_RMID);

    return 0;
}
