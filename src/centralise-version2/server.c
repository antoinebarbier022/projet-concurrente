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
#include<regex.h>
#include <cstring>
#include<signal.h>
#include <time.h>

using namespace std;

// variable globale pour le signal
int nbNotif = 0;
struct SystemState_s* p_att;
int sem_idNotif, sem_id, shm_id;



void signal_callback_handler(int signum){
    // Destruction du segment mémoire
    shmctl(shm_id,0,IPC_RMID);
    semctl(sem_id,0,IPC_RMID);
    semctl(sem_idNotif,0,IPC_RMID);
    cout << "\nDestruction Segment mémoire ..." << endl;
    cout << "Fermeture du server\n" << endl;
    exit(signum);
}

void* threadAffichageSysteme(void* p){
    while(1){
        struct sembuf lireNotif = {(u_short)0,(short)-1,SEM_UNDO};
        semop(sem_idNotif,&lireNotif,1);
        if(semctl(sem_idNotif, 0, GETVAL) == -1){
            //Information : Fermeture du thread affichage car le semaphore de notification n'existe plus.
            pthread_exit(p);
        } 

        lockSysteme(sem_id); // lock
            printf("\e[1;1H\e[2J");// efface la console
            afficherSysteme(p_att);
        unlockSysteme(sem_id); //unlock

        time_t s = time(NULL);
        struct tm* current_time = localtime(&s);
        printf(BWHT "\n Dernière Modification : ");
        printf("%02d:%02d.%02d\n",
           current_time->tm_hour,
           current_time->tm_min,
           current_time->tm_sec);

        lockSysteme(sem_id);
            printf(" Nombre de client : %d\n\n", p_att->nbClientConnecte);
        unlockSysteme(sem_id);

        printf(reset "Appuyer sur une touche pour arreter le server ...\n" );
    }
}

int main(int argc, char const *argv[])
{
    // pour gérer le ctrl-c et détruire le segment système
    signal(SIGINT, signal_callback_handler);

    printf("Lancement du server\n");
    printf("Importation des sites depuis le fichier sites.txt ...\n");
    sleep(1); // sleep pour ne pas afficher les messages trop rapidement
    
    SystemState_s etatSysteme;
    if(initSysteme(&etatSysteme,"sites.txt") == -1){
        return -1;
    }

    key_t cle = ftok(argv[1], 'e');                 // récuperer l'identifiant du segment mémoire
    key_t cle_sem = ftok(argv[1], 'u');             // récuperer l'identifiant du tableau sémaphores site
    key_t cle_semNotif = ftok("notif.txt", 'n');    // récuperer l'identifiant du tableau sémaphores  notification
    
    // création du segment mémoire pour l'état du système
    shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666); 
    if(shm_id == -1) {
        perror(BRED "Erreur [shmget] : La création du segment mémoire" reset);
        return -1;
    }else{
        printf("Création du segment mémoire [%d]\n",shm_id);
    }

    // création du tableau de sémaphores des ressources des sites
    int nbSemaphore = 4*etatSysteme.nbSites +1; 
    sem_id = semget(cle_sem, nbSemaphore, IPC_CREAT|0666);
    if(sem_id == -1) {
        perror(BRED "Erreur [semget] : La création du tableau de semaphores (ressources des sites)" reset);
        return -1;
    }
    
    // création du tableau de sémaphores des notifications client
    int nbNotif = NBCLIENTMAX; // nombre d'utilisateur max
    sem_idNotif = semget(cle_semNotif, nbNotif, IPC_CREAT|0666);
    if(sem_id == -1) {
        perror(BRED "Erreur [semget]: La création du tableau de semaphores (notifications)" reset);
        return -1;
    }

    // Initialisation du tableau de sémaphore avec les ressources des sites + le sémaphore 0 qui protège le segment mémoire
    if(initTableauSemSites(sem_id,&etatSysteme) == -1){
        perror(BRED "Erreur : L'initialisation du tableau de semaphores (ressources des sites)" reset);
        return -1;
    }

    
    union semun{
        int val;                // cmd= SETVAL
        struct semid_ds *buf;   // cmd= IPC_STAT ou IPC_SET
        unsigned short *array;  // cmd= GETALL ou SETALL
        struct seminfo *__buf;  // cmd= IPC_INFO (sous linux)
    }egCtrl;
    
    // On initialise le tableau de sémaphore notification à 0
    egCtrl.val =0;
    for(int i=0;i<NBCLIENTMAX;i++){
        if(semctl(sem_idNotif, i, SETVAL, egCtrl) == -1){  
            perror(BRED "Erreur : initialisation [sémaphore 0]" reset);
            return -1;
        }
    }

    // attachement au segement memoire
    p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror(BRED "Erreur [shmat] : L'attachement au segment mémoire" reset);
        exit(1);
    }

    // on initialise le segment de mémoire pour qu'il contienne l'état du système
    *p_att = etatSysteme;

    // Création du thread qui s'occupe de l'affichage quand il y a une notification
    pthread_t thread;
    if(pthread_create (&thread, NULL, threadAffichageSysteme, NULL)){
        printf(BRED "Erreur [pthread_create]: Création du thread Affichage" reset);
    }

    printf("Affichage du système :\n");
    afficherSysteme(&etatSysteme);

    printf("Appuyer sur une touche pour arreter le server ... ");

    // Attendre une réponse pour arrêter le server
    int touche;
    cin >> touche;

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
    }

    // Destruction du segment mémoire
    shmctl(shm_id,0,IPC_RMID);
    semctl(sem_id,0,IPC_RMID);
    semctl(sem_idNotif,0,IPC_RMID);
    
    cout << endl << "Destruction des objets IPC ..." << endl;
    cout << "Fermeture du server\n" << endl;
    
    return 0;
}
