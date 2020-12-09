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

int enAttenteRessource = 0;
int attenteTypeAction = 0;
int continuerDemande = 0;

int dS, dSNotif;

Requete_s ressourceLoue;

// variable globale pour les reponses du server
int enAttente = 1; // on est en attente tant qu'on ne reçoit pas la confirmation du server

// signal ctrl-c
void signal_callback_handler(int signum){
    
    // envoyer requete au server pour dire qu'on quitte
    if(close(dS) == 0){
        printf("fermeture socket parent réussi\n");
    }else{
        printf("erreur : fermeture socket parent\n");
    }

    if(close(dSNotif) == 0){
        printf("fermeture socket notif réussi\n");
    }else{
        printf("erreur : fermeture socket notif\n");
    }
    printf(BRED "\nFin du programme : Fermeture du client via ctrl-c\n" reset);
    exit(signum);
}


void* affichage(void* p){

    struct data{
        int type; // erreur -1 ou notif 0
        struct SystemState_s etatSysteme;
    }message;

    while(1){
        // reception de la notification
        // on lit d'abord le type pour savoir quoi faire
        if(recv(dSNotif, &message, sizeof(int),0) == 0){
            if(close(dSNotif) == 0){
                printf("fermeture socket notif réussi\n");
            }
            if(close(dS) == 0){
                printf("fermeture socket parent réussi\n");
            }
            printf(BRED "\nErreur : Le server ne répond plus\n" reset);
            exit(0);
        }
        printf("message type : %d",message.type);
        switch (message.type){
        case 1:
            enAttenteRessource = 0; // la demande à été accepté
        case 0 :
            // On receptionne l'état du système à jour
            recv(dSNotif, &message.etatSysteme.nbSites, sizeof(int),0);
            for(int nS=0;nS<message.etatSysteme.nbSites;nS++){ // num site
                recv(dSNotif, &message.etatSysteme.sites[nS].id, sizeof(int),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].nom, sizeof(message.etatSysteme.sites[nS].nom),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].cpu, sizeof(int),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].sto, sizeof(int),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].resCpuExFree, sizeof(float),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].resCpuShFree, sizeof(float),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].resStoExFree, sizeof(float),0);
                recv(dSNotif, &message.etatSysteme.sites[nS].resStoShFree, sizeof(float),0);
            }
            break;
        
        case -1:
            printf("Message : Vous êtes déconnecté du server suite à un problème technique.\n");
            if(close(dS) == 0){
                printf("fermeture socket parent réussi\n");
            }else{
                printf("erreur : fermeture socket parent\n");
            }

            if(close(dSNotif) == 0){
                printf("fermeture socket notif réussi\n");
            }else{
                printf("erreur : fermeture socket notif\n");
            }
            exit(0);
            break;
        }

        printf("\e[1;1H\e[2J");// efface la console
        time_t s = time(NULL);
        struct tm* current_time = localtime(&s);
        printf(BMAG "[Notification reçut à %02d:%02d.%02d !]\n",current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
        printf(reset);
        afficherSysteme(&message.etatSysteme);

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


int main(int argc, char const *argv[])
{

        // pour gérer le ctrl-c
    signal(SIGINT, signal_callback_handler);

    if(argc !=4){
        printf("lancement : ./client nomClient ip port\n");
        exit(0);
    }
    struct sockaddr_in serverAddr;

    // Creer une socket
    dS = socket(PF_INET, SOCK_STREAM,0);
    if(dS == -1){
        printf("Erreur : Création de la socket\n");
        exit(0);
    }else{
        printf("Création de la socket réussie\n");
    }

    // assigner IP et PORT
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[2]);    // IP
    serverAddr.sin_port = htons(atoi(argv[3]));          // PORT

    //int res = inet_pton(AF_INET,argv[1],&(serverAddr.sin_addr));
    socklen_t lgAdr = sizeof(struct sockaddr_in);

    // demander une connexion
    if(connect(dS, (struct sockaddr *)&serverAddr, lgAdr) != 0){
        printf("Erreur : la connexion avec le server à échoué\n");
        exit(0);
    }else{
        printf("Connexion avec le server réussie\n");
    }

    // Communication
            int port;
            
            // Creer une socket pour les notif
            dSNotif = socket(PF_INET, SOCK_STREAM,0);
            if(dSNotif == -1){
                printf("Erreur : Création de la socket pour envoyer les notifs\n");
                exit(0);
            }else{
                printf("Création de la socket pour envoyer les notif\n");
            }
            struct sockaddr_in serverNotifAddr;
            // assigner IP et PORT
            serverNotifAddr.sin_family = AF_INET;
            serverNotifAddr.sin_addr.s_addr = inet_addr(argv[2]);    // IP
            //serverNotifAddr.sin_port = htons(atoi(port));          // PORT
            recv(dS, &serverNotifAddr.sin_port, sizeof(serverNotifAddr.sin_port),0);

            socklen_t lgA = sizeof(struct sockaddr_in);
            // demander une connexion
            if(connect(dSNotif, (struct sockaddr *)&serverNotifAddr, lgA) != 0){
                printf("Erreur : la connexion avec le server à échoué\n");
            }else{
                printf("Connexion avec le client réussie\n");
            }
            




    

    // envoie du nom de l'utilisateur au server
    char nomClient[30];
    strcpy(nomClient,argv[1]);
    send(dS, &nomClient, sizeof(nomClient),0);

    
    SystemState_s etatSysteme;

    // Reception de l'état du système 
    recv(dS, &etatSysteme.nbSites, sizeof(int),0);
    for(int nS=0;nS<etatSysteme.nbSites;nS++){ // num site
        recv(dS, &etatSysteme.sites[nS].id, sizeof(int),0);
        recv(dS, &etatSysteme.sites[nS].nom, sizeof(etatSysteme.sites[nS].nom),0);
        recv(dS, &etatSysteme.sites[nS].cpu, sizeof(int),0);
        recv(dS, &etatSysteme.sites[nS].sto, sizeof(int),0);
        recv(dS, &etatSysteme.sites[nS].resCpuExFree, sizeof(float),0);
        recv(dS, &etatSysteme.sites[nS].resCpuShFree, sizeof(float),0);
        recv(dS, &etatSysteme.sites[nS].resStoExFree, sizeof(float),0);
        recv(dS, &etatSysteme.sites[nS].resStoShFree, sizeof(float),0);
    }
    

    // Affichage du nom d'utilisateur
    printf(BWHT);
        printf("Nom d'utilisateur  : %s\n", nomClient);
    printf(reset);

    // Afficher l'état du système
    afficherSysteme(&etatSysteme);










    




    // Création threads
    pthread_t thread;
    if(pthread_create (&thread, NULL, affichage, NULL)){
        printf("Erreur: Impossible de créer le thread d'affichage");
        return -1;
    }










    // Sur le client comme sur le server on fait une sauvegarde des requêtes en cours de reserveation
    // Cependant celle si sert juste pour l'affichage alors que sur le server on l'utilise aussi pour libérer
    // toutes les ressources du client, quand il quitte
    ressourceLoue.nbDemande = 0;
    for(int i=0;i<NBSITEMAX;i++){
        ressourceLoue.tabDemande[i].idSite = -1;
    }


    // la suite du code est equivalant à faire les actions suivantes :
        // while different de quitter
            // remplir la requete
            // envoyer la requete
            // attendre un message du server pour dire que la requete est accepté


    // initialisation de la requête
    Requete_s requete;
    requete.nbDemande = 0;
    requete.type = 1;

    // on fait des demande tant que le client ne souhaite pas quitter

    while(requete.type != 3 ){

        attenteTypeAction = 1; // pour l'affichage
            requete.type = saisieTypeAction();
        attenteTypeAction = 0; // pour l'affichage
        
        if(requete.type == 1 || requete.type == 2){
            int continuer = 1;
            do{
                if(requete.type == 1){
                    continuerDemande = 1; // pour l'affichage dans le thread

                    // on verifie en même temps que le site n'est pas déja louer
                    if(saisieDemandeRessource(&requete, &ressourceLoue) == -1){ // on saisie une demande
                        printf(BRED "Erreur : Saisie de la reservation non valide\n" reset);
                        
                        // ici faire en sorte de redemander correctement
                    } 
                }else{
                    if(ressourceLoue.nbDemande <=0){
                        printf(BRED "Erreur : Avant de pouvoir libérer des ressources, il faut en louer\n" reset);
                    
                        // ici faire en sorte de redemander correctement

                    }
                    continuerDemande = 2; // pour l'affichage dans le thread
                    // on verifie en même temps que le site est bien déja louer
                    if(saisieDemandeLiberation(&requete, &ressourceLoue) == -1){ // on saisie une demande
                        printf(BRED "Erreur : Saisie de la libération non valide\n" reset);
                        
                        // ici faire en sorte de redemander correctement

                    } 
                }


                printf("Faire une autre demande ? (Y/n) : ");
                char autreDemande[15];
                cin >> autreDemande;
                if(!cin){
                    printf(BRED "Erreur : La saisie est incorect\n" reset);
                    
                    // ici faire en sorte de redemander correctement


                }else{
                    if(strcmp(autreDemande,"Y") == 0){
                        // on vérifie que le nombre de demande n'est pas supérieur au nombre de site
                        if(requete.nbDemande >= etatSysteme.nbSites){ // NBSITEMAX car c'est le nombre de site max
                            printf(BWHT "Information : le nombre de demande ne peux pas être supérieur au nombre de site disponible.\n" reset);
                            continuer = 0;
                        }else{
                            continuer = 1;
                        }
                    }else if(strcmp(autreDemande,"n") == 0){
                        continuer = 0;
                    }else{
                        printf(BRED "Erreur : La saisie est incorect\n" reset);
                        continuer = 0;

                        // ici faire en sorte de redemander correctement
                    }
                }
                
            }while(continuer);
            continuerDemande = 0;  // boolean pour l'affichage dans le thread

            // Maintenant que la requete est remplie :
            // on envoie au server cette requête
            printf(BWHT);
                printf("\nEnvoie de la requête au server ...\n");
                printf("\nEn attente de la disponibilité des ressources demandé ...\n");
            printf(reset);

            // mettre un mutex plutot que ces variables

            enAttenteRessource = 1; // en attente des ressource
            send(dS,&requete,sizeof(struct Requete_s),0);
            updateRessourceLoueLocal(&requete,&ressourceLoue);
            // on doit mettre une boucle inini qui attent de savoir si la requete est accepté
            while(enAttenteRessource){
                // on reste blocké ici
            }
            

            
        resetRequete(&requete);
        }
        // pour quitter on envoie la requête avec le type 3
    }
    requete.type = 3;
    send(dS,&requete,sizeof(struct Requete_s),0);
    printf("Message fermeture envoyé\n");
    printf("Au revoir\n");
    
    
    //send(dS,&requete,sizeof(struct Requete_s),0);
    

    // fermer la sockets
    if(close(dS) == 0){
        printf("fermeture socket parent réussi\n");
    }else{
        printf("erreur : fermeture socket parent\n");
    }

    if(close(dSNotif) == 0){
        printf("fermeture socket notif réussi\n");
    }else{
        printf("erreur : fermeture socket notif\n");
    }


    return 0;
}
