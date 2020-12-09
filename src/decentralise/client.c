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
        printf(BRED "\nFermeture de la socket connecté au server" reset);
    }

    if(close(dSNotif) == 0){
        printf(BRED "\nFermeture de la socket connecté au server pour les notifications\n" reset);
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
        int res = recv(dSNotif, &message, sizeof(int),0);
        if(res == 0){ // la socket à été fermé
            printf(BRED "\nErreur : Le server ne répond plus\n" reset);
            if(close(dSNotif) == 0){
                printf(BRED "\nFermeture socket des notifs réussi" reset);
            }
            if(close(dS) == 0){
                printf(BRED "\nFermeture socket réussi\n" reset);
            }
            exit(0);
        }else if( res == -1){
            printf(BRED "\nErreur : recv notif = -1 \n" reset);
            exit(0);
        }

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
        
        default:
            break;
        }

        printf("\e[1;1H\e[2J");// efface la console
        time_t s = time(NULL);
        struct tm* current_time = localtime(&s);
        printf(BMAG "[Notification reçut à %02d:%02d.%02d !]\n",current_time->tm_hour,current_time->tm_min,current_time->tm_sec);
        printf(reset);
        afficherSysteme(&message.etatSysteme);

        if(enAttenteRessource == 1){
            printf("\nEn attente de la disponibilité des ressources demandées ...\n");
        }else{
            afficherRessourcesLoue(&ressourceLoue);
            if(attenteTypeAction == 1){
                printf("\nType action possible : \n - 1 : Demande de réservations");
                printf("\n - 2 : Demande de libérations\n - 3 : Quitter \nType action : \n");
            }else{
                if(continuerDemande == 1){
                    printf("\nContinuer la demande de ressources : \n");
                }else if(continuerDemande == 2){
                    printf("\nContinuer la demande de libérations : \n");
                }
            }
        }
    }
}


int main(int argc, char const *argv[]){

    // pour gérer le ctrl-c
    signal(SIGINT, signal_callback_handler);
    signal(SIGQUIT, signal_callback_handler);

    if(argc !=4){
        printf(BRED "lancement : ./client nomClient ip port\n" reset);
        exit(0);
    }
    struct sockaddr_in serverAddr;

    // Creer une socket
    dS = socket(PF_INET, SOCK_STREAM,0);
    if(dS == -1){
        printf(BRED "Erreur : Création de la socket\n" reset);
        exit(0);
    }

    // assigner IP et PORT
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[2]);    // IP
    serverAddr.sin_port = htons(atoi(argv[3]));          // PORT

    //int res = inet_pton(AF_INET,argv[1],&(serverAddr.sin_addr));
    socklen_t lgAdr = sizeof(struct sockaddr_in);

    // demander une connexion
    if(connect(dS, (struct sockaddr *)&serverAddr, lgAdr) != 0){
        printf(BRED "Erreur : La socket n'a pas pu se connecter au server (vérifie le port utilisé)\n" reset);
        exit(0);
    }

    // Communication
            
            // Creer une socket pour les notif
            dSNotif = socket(PF_INET, SOCK_STREAM,0);
            if(dSNotif == -1){
                printf(BRED "Erreur : Création de la socket pour envoyer les notifs\n" reset );
                exit(0);
            }

            int port;
            recv(dS, &port, sizeof(int),0);

            struct sockaddr_in serverNotifAddr;
            // assigner IP et PORT
            serverNotifAddr.sin_family = AF_INET;
            serverNotifAddr.sin_addr.s_addr = inet_addr(argv[2]);    // IP
            serverNotifAddr.sin_port = port;

            socklen_t lgA = sizeof(struct sockaddr_in);
            // demander une connexion
            //printf("port = %d\n", port);
            //printf("port = %d\n", serverNotifAddr.sin_port);
            if(connect(dSNotif, (struct sockaddr *)&serverNotifAddr, lgA) != 0){
                printf(BRED "Erreur : La connexion avec le server à échoué (socket notif)\n" reset);
                close(dS);
                close(dSNotif);
                exit(0);
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
        printf(BRED "Erreur: Impossible de créer le thread d'affichage" reset);
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

        // si on a déja louer tout ce qui était proposé
        if(ressourceLoue.nbDemande == etatSysteme.nbSites){
            if(requete.type == 1){
                printf(BWHT "Information : Tu as déjà reservé des ressources sur chacun des sites proposé.\n" reset);
                requete.type = 4; // autre chose que les actions possible afin de reposer la question
            }
        }
        
        if(requete.type == 1 || requete.type == 2){
            int continuer = 1;
            do{
                if(requete.type == 1){
                    continuerDemande = 1; // pour l'affichage dans le thread

                    
                        if(saisieDemandeRessource(&requete, &ressourceLoue, &etatSysteme) == -1){ // on saisie une demande
                            printf(BRED "Erreur : Saisie de la reservation non valide\n" reset);
                            // si on est ici c'est qu'il y a un gros problème car sinon ça redemande à l'utilisateur sa requête

                            // on quitte proprement
                            if(close(dS)  == 0){
                                printf(BRED "\nFermeture de la socket connecté au server" reset);
                            }
                            if(close(dSNotif) == 0){
                                printf(BRED "\nFermeture de la socket connecté au server" reset);
                            }
                            exit(0);
                        } 
                    
                }else{
                    if(ressourceLoue.nbDemande <=0){
                        printf(BRED "Erreur : Avant de pouvoir libérer des ressources, il faut en louer\n" reset);
                        
                        // on switch vers une demande
                        resetRequete(&requete);
                        requete.type = 1;
                        if(saisieDemandeRessource(&requete, &ressourceLoue, &etatSysteme) == -1){ // on saisie une demande
                            printf(BRED "Erreur : Saisie de la reservation non valide\n" reset);
                                // si on est ici c'est qu'il y a un gros problème car sinon ça redemande à l'utilisateur sa requête
                                // on quitte proprement
                                if(close(dS)  == 0){
                                    printf(BRED "\nFermeture de la socket connecté au server" reset);
                                }
                                if(close(dSNotif) == 0){
                                    printf(BRED "\nFermeture de la socket connecté au server" reset);
                                }
                                exit(0);
                        } 

                    }else{
                        continuerDemande = 2; // pour l'affichage dans le thread
                        // on verifie en même temps que le site est bien déja louer
                        if(requete.nbDemande == ressourceLoue.nbDemande){
                            printf(BWHT "Information : Tu as déjà fait toutes les demandes qu'il était possible de faire.\n" reset);
                        }else{
                            if(saisieDemandeLiberation(&requete, &ressourceLoue, &etatSysteme) == -1){ // on saisie une demande
                                printf(BRED "Erreur : Saisie de la libération non valide\n" reset);
                                
                                // si on est ici c'est qu'il y a un gros problème car sinon ça redemande à l'utilisateur sa requête
                                // on quitte proprement
                                if(close(dS)  == 0){
                                    printf(BRED "\nFermeture de la socket connecté au server" reset);
                                }
                                if(close(dSNotif) == 0){
                                    printf(BRED "\nFermeture de la socket connecté au server" reset);
                                }
                                exit(0);

                            } 
                        }

                    }

                }

                printf("Faire une autre demande ? (Y/n) : ");
                char autreDemande[15];
                cin >> autreDemande;
                if(!cin){
                    printf(BRED "Erreur : La saisie est incorect\n" reset);
                    printf(BRED "         On considère que vous ne souhaitez pas faire d'autre demandes.\n" reset);
                    sleep(2); // temps pour voir le message d'erreur et pour comprendre son erreur
                    continuer = 0;
                    
                }else{
                    if(strcmp(autreDemande,"Y") == 0){
                        // on vérifie que le nombre de demande n'est pas supérieur au nombre de site
                        if(requete.nbDemande >= etatSysteme.nbSites){ // NBSITEMAX car c'est le nombre de site max
                            printf(BWHT "Information : le nombre de demande ne peux pas être supérieur au nombre de sites disponible.\n" reset);
                            sleep(2); // temps pour voir le message d'erreur et pour comprendre son erreur
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
                printf("\nEnvoi de la requête au server ...\n");
                sleep(1); // pour permettre de voir les messages précédent (sinon la notif va les effacer)
                printf("\nEn attente de la disponibilité des ressources demandées ...\n");
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
            sleep(1); // pour permettre de laisser un peu de temps avant de redemander une saisie

        }
        // pour quitter on envoie la requête avec le type 3
    }
    requete.type = 3;
    send(dS,&requete,sizeof(struct Requete_s),0);

    printf(BWHT"\nAu revoir !\n" reset);
    

    

    // fermer la sockets
    close(dS);

    close(dSNotif);

    return 0;
}
