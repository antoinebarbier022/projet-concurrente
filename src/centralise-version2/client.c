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
#include<signal.h>


using namespace std;

// Variables global (pour pouvoir les utiliser dans le signal)
int idClient, numSemNotif;
int shm_id, sem_id, sem_idNotif;
int nbNotif = 0;
struct SystemState_s* p_att;
Requete_s ressourceLoue;

// signal ctrl-c
void signal_callback_handler(int signum){

        p_att->tabNotif[numSemNotif] = -1; // supprimer le semaphore de l'utilisateur
    char buffer[30] = {'\0'};
    strcpy(p_att->tabId[idClient-1],buffer);

    // libération des ressources
    lockSysteme(sem_id);
        ressourceLoue.type = 2; // libération
        updateSysteme(p_att, &ressourceLoue);
    unlockSysteme(sem_id);

    // envoie Notification car on a libérer les ressources avant de quitter
    if(emmetreNotif(sem_idNotif) == -1){
        perror(BRED "Erreur : lors de l'envoie des notifications" reset);
    }

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
        exit(1);
    }

    // Destruction du segment mémoire
    //lockSysteme(shm_id);
        //p_att->tabNotif[numSemNotif] = -1; // supprimer le semaphore de l'utilisateur
        //char buffer[30] = {'\0'};
        //strcpy(p_att->tabId[idClient-1],buffer);
    //unlockSysteme(shm_id);
    cout << "Fermeture du client via ctrl-c\n" << endl;
    exit(signum);
}

// Ma fonction Thread
// passer en parametre le l'id segment memoire, le numero
    
    void* threadAffichageSysteme(void* p){
        while(1){
            struct sembuf lireNotif = {(u_short)numSemNotif,(short)-1,SEM_UNDO};
            semop(sem_idNotif,&lireNotif,1);
            if(semctl(sem_idNotif, numSemNotif, GETVAL) == -1){
                pthread_exit(p);
            } 
            lockSysteme(sem_id); // lock
                printf("\e[1;1H\e[2J");// efface la console
                printf(BMAG "[Notification reçut !]\n" reset);
                afficherSysteme(p_att);
            unlockSysteme(sem_id); //unlock
            afficherRessourcesLoue(&ressourceLoue);
            
        }
    }

    
int main(int argc, char const *argv[])
{   
    signal(SIGINT, signal_callback_handler);
    if(argc != 3){
        printf(BRED "\nlancement : ./server chemin_fichier_cle nomClient\n\n" reset);
        return -1;
    }

    key_t cle = ftok(argv[1], 'e'); // récuperer l'identifiant ddu segment mémoire
    key_t cle_sem = ftok(argv[1], 'u'); // tableau site
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
        printf(BRED "\nMessage : Impossible de ce connecter au server\n" reset);
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

    //Algorithme du client :

    // Quand un client ce connecte : 
    //   - lui donner un identifiant (prendre la premiere case vide du tableau)
    //   - lui donner un numero de semaphore de notification

    printf("\e[1;1H\e[2J");// efface la console
    const char* nomClient = argv[2];
    lockSysteme(sem_id);
        idClient =      attribuerIdentifiantClient(p_att, nomClient);
        numSemNotif =   attribuerNumSemNotification(p_att, idClient);
        p_att->nbClientConnecte++;

        printf(BWHT);
            printf("Nom d'utilisateur : %s\n", nomClient);
            printf("Id client : %d \n",idClient);
            printf("Num semNotif : %d \n",numSemNotif);
        printf(reset);

        afficherSysteme(p_att);
    unlockSysteme(sem_id);//unlock

    egCtrl.val = 0;
    if(semctl(sem_idNotif, numSemNotif, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
        perror(BRED "Erreur : initialisation [sémaphore 0]" reset);
        return -1;
    }

    pthread_t thread;
    if(pthread_create (&thread, NULL, threadAffichageSysteme, NULL)){
        printf("Erreur: Impossible de créer le thread d'affichage");
        exit(-1);
    }

    //Requete_s ressourceLoue; // tableau qui contient tous les sites déjà reservé
    ressourceLoue.nbDemande = 0;
    ressourceLoue.idClient = idClient;
    for(int i=0;i<NBSITEMAX;i++){
        ressourceLoue.tabDemande[i].idSite = -1;
    }
    
    int typeDemande = 1; // 
    while(typeDemande != 3 ){
        typeDemande = saisieTypeAction();
        Requete_s requete;
        requete.nbDemande = 0;
        requete.type = typeDemande;
        requete.idClient = idClient;
        

        //requete.nbDemande = 1;
        //requete.tabDemande[0]={2,2,7,0};
        if(typeDemande != 3){
            int continuer = 1;
            do{
                if(typeDemande == 1){
                    // on verifie en meme temps que le site n'est pas déja louer
                    saisieDemandeRessource(&requete, &ressourceLoue); // on saisie une demande
                }else{
                    if(ressourceLoue.nbDemande <=0){
                        printf(BRED "Erreur : Avant de pouvoir libérer des ressources, il faut en louer." reset);
                        exit(1); 
                    }
                    // on verifie en meme temps que le site est bien déja louer
                    saisieDemandeLiberation(&requete, &ressourceLoue);
                }
                printf("Faire une autre demande ? (Y/n) : ");
                char autreDemande[15];
                cin >> autreDemande;
                if(!cin){
                    printf(BRED "Erreur : La saisie est incorect\n" reset);
                    exit(1);
                }else{
                    if(strcmp(autreDemande,"Y") == 0){
                        continuer = 1;
                    }else if(strcmp(autreDemande,"n") == 0){
                        continuer = 0;
                    }else{
                        printf(BRED "Erreur : La saisie est incorect\n" reset);
                        continuer = 0;
                        exit(1);
                    }
                }
                
            }while(continuer);
        }

        if(typeDemande == 1 || typeDemande == 2){
            printf("\n\nVérification de la requête ...\n");
            lockSysteme(sem_id); //lock
                if(demandeValide(p_att, &requete) == 1){
                    //preparer les opérations sembuf
                    struct sembuf *op = (struct sembuf*)malloc(4*p_att->nbSites*sizeof(struct sembuf));
                    //struct sembuf op[NBOPMAXRESSOURCE];
                    int nbOp = initOp(op,&requete);
                    unlockSysteme(sem_id); // unlock
                        printf("Attente ...\n");
                        semop(sem_id,op,nbOp);
                        printf("Demande accepté !\n");
                    lockSysteme(sem_id); //lock
                    //updateSystem2(p_att, sem_idNotif);
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
        sleep(1);
    }
    printf("Au revoir\n");

    // On supprime toutes les infos client du systeme (donc ses reservations, son attribution à un sem de notification et à un id ...)
    //supprimerClient();
    p_att->tabNotif[numSemNotif] = -1; // supprimer le semaphore de l'utilisateur
    char buffer[30] = {'\0'};
    strcpy(p_att->tabId[idClient-1],buffer);

    // libération des ressources
    lockSysteme(sem_id);
        ressourceLoue.type = 2; // libération
        updateSysteme(p_att, &ressourceLoue);
    unlockSysteme(sem_id);

    // envoie Notification car on a libérer les ressources avant de quitter
    if(emmetreNotif(sem_idNotif) == -1){
        perror(BRED "Erreur : lors de l'envoie des notifications" reset);
    }

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
        exit(1);
    }

    return 0;
}
