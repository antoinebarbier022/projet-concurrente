#include "struct.h"
#include "fonctions.h"

#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <regex.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <iostream>

using namespace std;


// Variables global (pour pouvoir les utiliser dans le signal)

int idClient, numSemNotif;
int shm_id, sem_id, sem_idNotif;
int nbNotif = 0;

SystemState_s* p_att;
Requete_s ressourceLoue;


int dS, dSClient;
int dSNotif;
pid_t pid = 1;
int demandeRessourceAccepte;

void signal_callback_handler(int signum){
    if(close(dS) == 0){
        printf(BRED "fermeture socket réussi\n" reset);
    }
    if(close(dSClient) == 0){
        printf(BRED "fermeture socket réussi\n" reset);
    }
    if(close(dSNotif) == 0){
        printf(BRED "fermeture socket réussi\n" reset);
    }
    if(pid != 0){ 
 // si je suis le parent car seul le parent peut détruire les objets IPC

        // détachement du segment mémoire
        int dtres = shmdt(p_att); 
        if(dtres == -1){
            perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
        }

        // Destruction du segment mémoire
        printf(BRED "\n\nDestruction des objets IPC ...\n" reset);
        printf(BRED "Fermeture du server\n" reset);
        shmctl(shm_id,IPC_RMID,NULL);
        semctl(sem_id,0,IPC_RMID);
        semctl(sem_idNotif,0,IPC_RMID);
        
    }
    exit(signum);
}


void* notification(void* p){
    struct data{
        int type; // erreur -1 ou notif 0
        SystemState_s etatSysteme;
    }message;
    while(1){
        // Lire une notification quand elle apparait
        struct sembuf lireNotif = {(u_short)numSemNotif,(short)-1,SEM_UNDO};
        semop(sem_idNotif,&lireNotif,1);

        lockSysteme(sem_id); // lock
        if(demandeRessourceAccepte == 1){
            message.type = 1; // c'est la demande du client qui a été accepté
        }else{
            message.type = 0; // c'est une notif
        }
            
            message.etatSysteme = *p_att;
        unlockSysteme(sem_id); //unlock

        // mettre un mutex ici car on peut envoyé aussi des messages d'erreur, il faut pas que les
        //   deux soient envoyé en même temps
        
        // envoie du message au client
        // on envoie d'abord le type
        send(dSNotif, &message.type, sizeof(int),0);
        // On envoie l'état du système au client
        send(dSNotif, &message.etatSysteme.nbSites, sizeof(int),0);
        for(int nS=0;nS<message.etatSysteme.nbSites;nS++){ // num site
            send(dSNotif, &message.etatSysteme.sites[nS].id, sizeof(int),0);
            send(dSNotif, &message.etatSysteme.sites[nS].nom, sizeof(message.etatSysteme.sites[nS].nom),0);
            send(dSNotif, &message.etatSysteme.sites[nS].cpu, sizeof(int),0);
            send(dSNotif, &message.etatSysteme.sites[nS].sto, sizeof(int),0);
            send(dSNotif, &message.etatSysteme.sites[nS].resCpuExFree, sizeof(float),0);
            send(dSNotif, &message.etatSysteme.sites[nS].resCpuShFree, sizeof(float),0);
            send(dSNotif, &message.etatSysteme.sites[nS].resStoExFree, sizeof(float),0);
            send(dSNotif, &message.etatSysteme.sites[nS].resStoShFree, sizeof(float),0);
        }
    }
}

void* terminerProgramme(void* p){
    key_t cle = ftok("IPC/ipc.txt", 'e');
    while(1){
        // si jamais un des objets ipc est tué, on arrete le programme
        if(semctl(sem_idNotif, 0, GETVAL) == -1 || semctl(sem_id, 0, GETVAL) == -1 ||  shmget(cle, sizeof(SystemState_s), 0666) == -1){
            if(close(dS) == 0){
                printf("fermeture socket parent réussi\n");
            }
            if(close(dSNotif) == 0){
                printf("fermeture socket notif réussi\n");
            }
            if(close(dSClient) == 0){
                printf("fermeture socket fils réussi\n");
            }

            // détachement du segment mémoire
            int dtres = shmdt(p_att); 
            if(dtres == -1){
                perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
                exit(1);
            }
            shmctl(shm_id,IPC_RMID, NULL);
            semctl(sem_id,0,IPC_RMID);
            semctl(sem_idNotif,0,IPC_RMID);
            exit(0);
        }
    }
}


int main(int argc, char const *argv[])
{
    signal(SIGINT, signal_callback_handler);
    signal(SIGQUIT, signal_callback_handler);
    if(argc !=2){
        printf("lancement : ./server port\n");
        exit(0);
    }

    printf("Lancement du server\n");
    printf("Importation des sites depuis le fichier sites.txt ...\n");
    sleep(1); // sleep pour ne pas afficher les messages trop rapidement
    

    // Récupération des sites 
    SystemState_s etatSysteme;
    if(initSysteme(&etatSysteme,"sites.txt") == -1){
        return -1;
    }

    // identification des clés pour les objets IPC
    key_t cle = ftok("IPC/ipc.txt", 'e');                 // récuperer l'identifiant du segment mémoire
    key_t cle_sem = ftok("IPC/ipc.txt", 'u');             // récuperer l'identifiant du tableau sémaphores site
    key_t cle_semNotif = ftok("IPC/notif.txt", 'n');    // récuperer l'identifiant du tableau sémaphores  notification
    
    // création du segment mémoire pour l'état du système
    shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666); 
    if(shm_id == -1) {
        perror(BRED "Erreur [shmget] : La création du segment mémoire" reset);
        return -1;
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

    // afficher l'état du système
    printf("Affichage du système :\n");
    afficherSysteme(&etatSysteme);



// Creer une socket
    dS = socket(PF_INET, SOCK_STREAM,0);
    if(dS == -1){
        printf("Erreur : Création de la socket\n");
        exit(0);
    }
    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1]));

    // Nommage de la socket
    bind(dS, (struct sockaddr*)&ad, sizeof(ad));

    // Passer la socket en mode écoute
    int res = listen(dS, NBSITEMAX);
    if(res == -1){
        printf("Erreur : listen\n");
        exit(0);
    }

    struct sockaddr_in adClient;
    socklen_t lgAdr = sizeof(struct sockaddr_in); 

    // Accepter une connexion
    //pthread_t thread;
    int mavariable = 1;
    while(mavariable){
        dSClient = accept(dS,(struct sockaddr*)&adClient,&lgAdr);
        if(dSClient == -1){
            printf(BRED "Erreur : accept\n" reset);
            close(dS);
            close(dSClient);
            exit(0);
        }
        pid = fork();
        if(pid == 0){

            // je veux maintenant une socket pour les notif
            dSNotif = socket(PF_INET, SOCK_STREAM,0);
            if(dSNotif == -1){
                printf("Erreur : Création de la socket de notif\n");
                close(dSClient);
                close(dSNotif);
                exit(0);
            }
            struct sockaddr_in adS;
            adS.sin_family = AF_INET;
            adS.sin_addr.s_addr = INADDR_ANY;
            adS.sin_port = 0; // on utilise un port random
            
            
            // Nommage de la socket
            bind(dSNotif, (struct sockaddr*)&adS, sizeof(adS));
            socklen_t lgAN = sizeof(struct sockaddr_in);
            getsockname(dSNotif, (struct sockaddr*)&adS, &lgAN);
            int port = (int)adS.sin_port;
            send(dSClient, &port, sizeof(int),0);

            // j'attent le port et l'ip client pour me connecter à lui avec une socket
            
            // Passer la socket en mode écoute
            int res = listen(dSNotif, NBSITEMAX);
            if(res == -1){
                printf("Erreur : listen\n");
                close(dSClient);
                close(dSNotif);
                exit(0);
            }

            dSNotif = accept(dSNotif,(struct sockaddr*)&adS,&lgAN);
            if(dSNotif <=0){
                printf("\nproblème accept\n");
                close(dSClient);
                close(dSNotif);
                exit(0);
            }

    
        

            // Reception du nom d'utilisateur du client
            char nomClient[30];
            recv(dSClient, &nomClient, sizeof(nomClient),0);
                
            printf(BWHT);
                printf("%s viens de se connecter au server !\n", nomClient);
            printf(reset);

            // J'associe mon client avec un id et un sémaphore
            lockSysteme(sem_id);
                idClient =      attribuerIdentifiantClient(p_att, nomClient);
                numSemNotif =   attribuerNumSemNotification(p_att, idClient);
                p_att->nbClientConnecte++;
                // On fait une copie de l'état du système
                SystemState_s etatSystemeCopy = *p_att;
            unlockSysteme(sem_id);//unlock

            afficherSysteme(&etatSystemeCopy);
            

            // On envoie l'état du système au client
            send(dSClient, &etatSystemeCopy.nbSites, sizeof(int),0);
            for(int nS=0;nS<etatSystemeCopy.nbSites;nS++){ // num site
                send(dSClient, &etatSystemeCopy.sites[nS].id, sizeof(int),0);
                send(dSClient, &etatSystemeCopy.sites[nS].nom, sizeof(etatSystemeCopy.sites[nS].nom),0);
                send(dSClient, &etatSystemeCopy.sites[nS].cpu, sizeof(int),0);
                send(dSClient, &etatSystemeCopy.sites[nS].sto, sizeof(int),0);
                send(dSClient, &etatSystemeCopy.sites[nS].resCpuExFree, sizeof(float),0);
                send(dSClient, &etatSystemeCopy.sites[nS].resCpuShFree, sizeof(float),0);
                send(dSClient, &etatSystemeCopy.sites[nS].resStoExFree, sizeof(float),0);
                send(dSClient, &etatSystemeCopy.sites[nS].resStoShFree, sizeof(float),0);
            }



            // J'initialise le sémaphore notification du client à 0
            egCtrl.val = 0;
            if(semctl(sem_idNotif, numSemNotif, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
                perror(BRED "Erreur : initialisation sémaphore" reset);
                return -1;
            }



            // Je créer mes deux threads
            // - celui pour envoyer un message avec l'état du système quand il y a une notification
            // - celui qui envoie un message au client si le server ne va pas bien (ex : objet ipc qui n'existe plus)


            
            struct param_thread{
                int *dSNotif;
                int *dS;
            } args;
            args.dSNotif = &dSNotif;
            args.dS = &dS;
            
            pthread_t threadNotification;
            if(pthread_create (&threadNotification, NULL, notification, &args)){
                printf("Erreur: Impossible de créer le thread de notification");
                return -1;
            }

            pthread_t threadFinProgramme;
            if(pthread_create (&threadFinProgramme, NULL, terminerProgramme, &args)){
                printf(BRED "Erreur [pthread_create]: Création du thread finProgramme" reset);
                return -1;
            }
            


            // Initialisation de ma structure qui contient toutes les ressources que j'ai réservé
            // le faire dans une fonction plus tard
            ressourceLoue.nbDemande = 0;
            ressourceLoue.idClient = idClient;
            for(int i=0;i<NBSITEMAX;i++){
                ressourceLoue.tabDemande[i].idSite = -1;
            }


            struct Requete_s requete;
            requete.idClient = idClient;
            requete.type = 1;

            // On boucle tant que le client n'envoie pas de requête où il demande de quitter
            while(requete.type != 3){

                // reception de la requête du client
                if(recv(dSClient, &requete, sizeof(requete),0) == 0){
                    close(dS);
                    close(dSNotif);
                    // Fermeture du client, on le supprime du segment mémoire
                    printf(BWHT);
                        printf("%s viens de quitter le server !\n", nomClient);
                    printf(reset);
                    // cette fonction supprime et fait appel à exit
                    fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);
                    afficherSysteme(&etatSysteme);
                }else{
                    printf(BWHT);
                        switch (requete.type){
                        case 1:
                            printf("%s viens d'envoyer une demande de ressource !\n", nomClient);
                            break;
                        case 2:
                            printf("%s viens d'envoyer une demande de libération !\n", nomClient);
                            break;
                        case 3:
                            printf("%s viens d'envoyer une demande de déconnexion !\n", nomClient);
                            break;
                        }
                    printf(reset);
                }
                
                
                if(requete.type != 3){
                    // On créer toutes les opérations sembuf  en fonction de la requête
                    lockSysteme(sem_id);
                        struct sembuf *op = (struct sembuf*)malloc(4*p_att->nbSites*sizeof(struct sembuf));
                        int nbOp = initOp(op,&requete);
                        // la demande n'est pas encore accepté
                        demandeRessourceAccepte = 0;
                    unlockSysteme(sem_id);

                    // execution du semop pour reserver les ressources
                    semop(sem_id,op,nbOp); 

                    // Mise à jour du système
                    lockSysteme(sem_id);
                        demandeRessourceAccepte = 1; // alors le code de la notif sera 1
                        updateSysteme(p_att, &requete);
                    unlockSysteme(sem_id);
                    updateRessourceLoueLocal(&requete,&ressourceLoue);
                    free(op);
                    afficherSysteme(p_att);

                    // Quand il y a une notif tous les semaphores sont à 1
                    // le code qui suit est l'emission d'une notification
                    if(emmetreNotif(sem_idNotif)== -1){
                        perror(BRED "Erreur : lors de l'envoi des notifications" reset);
                    }
                }
            }

            // Fermeture du client, on le supprime du segment mémoire
            printf(BWHT);
                printf("%s viens de quitter le server !\n", nomClient);
            printf(reset);
            close(dSClient);
            // cette fonction supprime et fait appel à exit
            fermerClient(idClient, numSemNotif, p_att, &ressourceLoue ,shm_id, sem_id, sem_idNotif);
            afficherSysteme(&etatSysteme);

        }else{
            
            // processus parent 
        }    
    }

    if(pid == 0){ // processus fils
        printf("Fin du programme fils\n");
        if(close(dS) == 0){
            printf(BRED "Fermeture socket réussi\n" reset);
        }
        if(close(dSClient) == 0){
            printf(BRED "Fermeture socket réussi\n" reset);
        }
    }else{ // si je suis le parent car seul le parent peut détruire les objets IPC
        // détachement du segment mémoire
        int dtres = shmdt(p_att); 
        if(dtres == -1){
            perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
        }

        // Destruction du segment mémoire
        printf(BRED "\n\nDestruction des objets IPC ...\n" reset);
        printf(BRED "Fermeture du server\n" reset);
        shmctl(shm_id,IPC_RMID,NULL);
        semctl(sem_id,0,IPC_RMID);
        semctl(sem_idNotif,0,IPC_RMID);
        
    }



    return 0;
}
