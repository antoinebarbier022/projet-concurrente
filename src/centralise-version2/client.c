#include "struct.h"
#include "fonctions.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

using namespace std;

// Variables global (pour pouvoir les utiliser dans le signal)
int idClient, numSemNotif;
int shm_id, sem_id, sem_idNotif;
int nbNotif = 0;
struct SystemState_s* p_att;
Requete_s ressourceLoue;

int enAttenteRessource = 0;
int attenteTypeAction = 0;
int continuerDemande = 0;

// signal ctrl-c
void signal_callback_handler(int signum){
    fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);
    printf(BRED "\nFin du programme : Fermeture du client via ctrl-c\n" reset);
    exit(signum);
}
    
void* threadAffichageSysteme(void* p){
    while(1){
        // Lire une notification quand elle apparait
        struct sembuf lireNotif = {(u_short)numSemNotif,(short)-1,SEM_UNDO};
        semop(sem_idNotif,&lireNotif,1);

        if(semctl(sem_idNotif, numSemNotif, GETVAL) == -1){
            // Si on arrive pas à lire le semaphore c'est qu'il y a un prblème, donc on quitte le thread
            pthread_exit(p);
        } 

        lockSysteme(sem_id); // lock
            printf("\e[1;1H\e[2J");// efface la console
            time_t s = time(NULL);
            struct tm* current_time = localtime(&s);
            printf(BMAG "[Notification reçut à %02d:%02d.%02d !]\n",current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
            printf(reset);
            afficherSysteme(p_att);
        unlockSysteme(sem_id); //unlock
        if(enAttenteRessource == 1){
            printf("\nEn attente de la disponibilité des ressources demandé ...\n");
        }else{
            afficherRessourcesLoue(&ressourceLoue);
            if(attenteTypeAction == 1){
                printf("\nType action possible : \n - 1 : Demande de réservation");
                printf("\n - 2 : Demande de libération\n - 3 : Quitter \nType action : \n");
            }else{
                if(continuerDemande == 1){
                    printf("\nContinuer la demande de ressources : \n");
                }else if(continuerDemande == 2){
                    printf("\nContinuer la demande de libération : \n");
                }
            }
        }
    }
}

void* threadTerminerProgramme(void* p){
    while(1){
        if(semctl(sem_idNotif, 0, GETVAL) == -1){
            //Information : Fermeture du thread affichage car le semaphore de notification n'existe plus.
            printf(BRED "\nFin du programme : le server ne répond plus\n" reset);

            // détachement du segment mémoire
            int dtres = shmdt(p_att); 
            if(dtres == -1){
                perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
                exit(1);
            }
            exit(0);
        } 
    }
}
    
int main(int argc, char const *argv[])
{   
    // pour gérer le ctrl-c
    signal(SIGINT, signal_callback_handler);

    if(argc != 2){
        printf(BRED "\nlancement : ./client nomClient\n\n" reset);
        return -1;
    }
    const char* nomClient = argv[1];

    key_t cle = ftok("ipc.txt", 'e'); // récuperer l'identifiant ddu segment mémoire
    key_t cle_sem = ftok("ipc.txt", 'u'); // tableau site
    key_t cle_semNotif = ftok("notif.txt", 'n'); // tableau notification
    
    // identification/création du segment mémoire pour l'état du système
    shm_id = shmget(cle, sizeof(SystemState_s), 0666); 
    if(shm_id == -1) {
        printf(BRED "\nMessage : Impossible de ce connecter au server\n" reset);
        printf(BRED "Erreur [shmget] : Connexion au segment mémoire\n\n" reset);
        exit(1);
    }

    //attachement au segement memoire
    p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        printf(BRED "\nMessage : Impossible de ce connecter au server \n" reset);
        printf(BRED "Erreur [shmat] : Attachement au segment mémoire\n\n" reset);
        exit(1);
    }

    // identification/création du tableau de sémaphores des ressources des sites
    int nbSemaphore = 4*p_att->nbSites +1; 
    sem_id = semget(cle_sem, nbSemaphore, 0666);
    if(sem_id == -1) {
        printf(BRED "\nMessage : Impossible de ce connecter au server\n" reset);
        printf(BRED "Erreur [semget] : Connexion au tableau de semaphores (ressources des sites)\n\n" reset);
        exit(1);
    }
    
    // identification/création du tableau de sémaphores des notifications client
    int nbNotif = NBCLIENTMAX; // nombre d'utilisateur max
    sem_idNotif = semget(cle_semNotif, nbNotif, 0666);
    if(sem_id == -1) {
        printf(BRED "\nMessage : Impossible de ce connecter au server\n" reset);
        printf(BRED "Erreur [semget]: Connexion au tableau de semaphores (notifications)\n\n" reset);
        exit(1);
    }

    union semun{
        int val;                // cmd= SETVAL
        struct semid_ds *buf;   // cmd= IPC_STAT ou IPC_SET
        unsigned short *array;  // cmd= GETALL ou SETALL
        struct seminfo *__buf;  // cmd= IPC_INFO (sous linux)
    }egCtrl;

    printf("\e[1;1H\e[2J");// efface la console
    lockSysteme(sem_id);
        idClient =      attribuerIdentifiantClient(p_att, nomClient);
        numSemNotif =   attribuerNumSemNotification(p_att, idClient);
        p_att->nbClientConnecte++;

        printf(BWHT);
            printf("Nom d'utilisateur  : %s\n", nomClient);
            printf("Identifiant        : %d \n",idClient);
            printf("Num semNotif       : %d \n",numSemNotif);
        printf(reset);

        afficherSysteme(p_att);
    unlockSysteme(sem_id);//unlock

    egCtrl.val = 0;
    if(semctl(sem_idNotif, numSemNotif, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
        perror(BRED "Erreur : initialisation sémaphore" reset);
        return -1;
    }

    pthread_t thread;
    if(pthread_create (&thread, NULL, threadAffichageSysteme, NULL)){
        printf("Erreur: Impossible de créer le thread d'affichage");
        return -1;
    }

    pthread_t finProgramme;
    if(pthread_create (&finProgramme, NULL, threadTerminerProgramme, NULL)){
        printf(BRED "Erreur [pthread_create]: Création du thread finProgramme" reset);
        return -1;
    }

    //Requete_s ressourceLoue; // tableau qui contient tous les sites déjà reservé
    ressourceLoue.nbDemande = 0;
    ressourceLoue.idClient = idClient;
    for(int i=0;i<NBSITEMAX;i++){
        ressourceLoue.tabDemande[i].idSite = -1;
    }
    
    int typeDemande = 1; // 
    while(typeDemande != 3 ){
        attenteTypeAction = 1; // pour l'affichage
            typeDemande = saisieTypeAction();
        attenteTypeAction = 0; // pour l'affichage
        Requete_s requete;
        requete.nbDemande = 0;
        requete.type = typeDemande;
        requete.idClient = idClient;
        
        if(typeDemande == 1 || typeDemande == 2){
            int continuer = 1;
            do{
                if(typeDemande == 1){
                    continuerDemande = 1; // pour l'affichage dans le thread

                    // on verifie en même temps que le site n'est pas déja louer
                    if(saisieDemandeRessource(&requete, &ressourceLoue) == -1){ // on saisie une demande
                        printf(BRED "Erreur : Saisie de la reservation non valide\n" reset);
                        return -1; 
                    } 
                }else{
                    if(ressourceLoue.nbDemande <=0){
                        printf(BRED "Erreur : Avant de pouvoir libérer des ressources, il faut en louer\n" reset);
                        return -1; 
                    }
                    continuerDemande = 2; // pour l'affichage dans le thread
                    // on verifie en même temps que le site est bien déja louer
                    if(saisieDemandeLiberation(&requete, &ressourceLoue) == -1){ // on saisie une demande
                        printf(BRED "Erreur : Saisie de la libération non valide\n" reset);
                        return -1; 
                    } 
                }
                printf("Faire une autre demande ? (Y/n) : ");
                char autreDemande[15];
                cin >> autreDemande;
                if(!cin){
                    printf(BRED "Erreur : La saisie est incorect\n" reset);
                    fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);
                }else{
                    if(strcmp(autreDemande,"Y") == 0){
                        continuer = 1;
                    }else if(strcmp(autreDemande,"n") == 0){
                        continuer = 0;
                    }else{
                        printf(BRED "Erreur : La saisie est incorect\n" reset);
                        continuer = 0;
                        fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);
                    }
                }
            }while(continuer);
            continuerDemande = 0;  // boolean pour l'affichage dans le thread

            printf("\n\nVérification de la requête ...\n");
            lockSysteme(sem_id); //lock
                if(demandeValide(p_att, &requete) == 1){
                    //preparer les opérations sembuf
                    struct sembuf *op = (struct sembuf*)malloc(4*p_att->nbSites*sizeof(struct sembuf));
                    int nbOp = initOp(op,&requete);
                    enAttenteRessource = 1; // je suis en attente 
                    printf("\nEn attente de la disponibilité des ressources demandé ...\n");
                    unlockSysteme(sem_id); // unlock
                        semop(sem_id,op,nbOp);
                    lockSysteme(sem_id); //lock
                    printf("Demande accepté !\n");
                    enAttenteRessource = 0; // je suis en attente 

                    updateSysteme(p_att, &requete);
                    updateRessourceLoueLocal(&requete,&ressourceLoue);
                    free(op);

                    // Quand il y a une notif tous les semaphores sont à 1
                    // le code qui suit est l'emission d'une notification
                    if(emmetreNotif(sem_idNotif)== -1){
                        perror(BRED "Erreur : lors de l'envoie des notifications" reset);
                    }
                }else{
                    //message d'erreur déja dans la fonction
                }
            unlockSysteme(sem_id);//unlock
        }
        // Ce sleep permet d'attendre 1seconde avant de redemander une saisie 
        sleep(1);
    }
    printf("Au revoir\n");

    // On supprime toutes les infos client du systeme (donc ses reservations, son attribution à un sem de notification et à un id ...)
    fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);

    return 0;
}
