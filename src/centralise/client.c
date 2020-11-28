#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NBMAXCLIENT 200
#define NBSITES 10 
#define NBDEMANDEMAX 20

void initStructModif(Modification_s *m){
    m->type = 0;
    m->nbDemande = 0;
}
// on l'appel une fois une modification terminé
void resetStructModif(Modification_s *m){
    m->type = 0;
    for(int i=0;i<m->nbDemande;i++){
        m->tabDemande[i].idSite = -1;
        m->tabDemande[i].mode = 0;
        m->tabDemande[i].cpu = 0;
        m->tabDemande[i].sto = 0;
    }
    m->nbDemande = 0;
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

void afficherStructureRequete(Modification_s *m){
    if(m->type == 1){
        printf("\n[Demande de ressources]");
    }else{
        printf("\n[Demande de libération de ressources]");
    }

    for(int i=0;i<m->nbDemande;i++){
        printf("\n [Site %d] : %d cpu, %.1f Go ",  
                m->tabDemande[i].idSite, 
                m->tabDemande[i].cpu, 
                m->tabDemande[i].sto);
        if(m->tabDemande[i].mode == 1){
            printf("[mode exclusif]");
        }else{
            printf("[mode partagé]");
        }
    }
    printf("\n");
}

void afficherRessourcesLoue(RessourceLoue_s *r){
    printf("\nRessource loués :");
    if(r->nbRessources != 0){
        int nbSite = NBSITES;
        for(int i=0;i<nbSite;i++){
            if(r->tabLocation[i].idSite != -1){
                printf("\n [Site %d] : %d cpu, %.1f Go ",  
                    r->tabLocation[i].idSite, 
                    r->tabLocation[i].cpu, 
                    r->tabLocation[i].sto);
                if(r->tabLocation[i].mode == 1){
                    printf("[mode exclusif]");
                }else{
                    printf("[mode partagé]");
                }
            }
        }
    }else{
        printf(" Aucune");
    }
    printf("\n");
}

void messageChoixTypeAction(){
    printf("\nVoici les actions possible :\n");
    printf("  1 : demande de ressources\n");
    printf("  2 : libération de ressources\n");
}

// Cette fonction demande à l'utilisateur quelle type d'action il veut faire (demande de ressource ou libération)
// La fonction retourne 1 ou 2 en fonction de l'action demandé
// 1 pour la demande de ressource et 2 pour la libération
int demandeTypeAction(){
    int type = 0;
    // Choix de l'action à effectué (demande de ressource ou bien libération)
    printf("\nQuelle action voulez-vous effectuer ? (1 ou 2) : ");
    if(scanf("%d",&type) != 1){
        return -1;
    }else{
        if(type == 1 || type == 2){
            return type;
        }else{
            return -1;
        }
    }
}

// Cette fonction demande les ressources à louer au client et les places dans la stucture Modification
void demandeDeRessources(Modification_s *m){
    int inputId, inputMode, inputCpu;
    float inputSto;
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
    }
    else{ // le format est correct mais cela ne veut pas dire que la demande l'est.
        //printf("[Demande de ressource] Site %d : %d cpu, %.1f Go en mode %d\n", inputId, inputCpu, inputSto, inputMode);
        // Sinon on entre la demande
        m->nbDemande++; // on a mtn une première demande
        if(inputMode == 2 || inputMode == 1){
            m->tabDemande[m->nbDemande - 1].mode = inputMode;
            m->tabDemande[m->nbDemande - 1].idSite = inputId;
            m->tabDemande[m->nbDemande - 1].cpu = inputCpu;
            m->tabDemande[m->nbDemande - 1].sto = inputSto;
        }else{
            perror("erreur : le mode doit être 1 pour <mode exclusif> ou 2 pour <mode partagé>");
            exit(1);
        }
    }
}

// on devra simplement rentrer le nom du site
// renvoie le site à libérer
int demandeDeLiberations(Modification_s *m, RessourceLoue_s *r){
    int inputIdSite;
    printf("\nSur quelle site veux tu libérer les ressources ?\n");
    if(scanf("%d",&inputIdSite) != 1){
        printf("Erreur : entrée non attendu");
         exit(1); 
    }else{
        int i = 0;
        while(i<r->nbRessources){
            if(r->tabLocation[i].idSite == inputIdSite){
                // on peut libérer cette ressource car elle a été loué
                return inputIdSite;
            }
            i++;
        }
        return -1; // Le site dont on demmande la libération n'a jamais été louer ou n'existe pas
    }
}

// on met à jour la struct local qui contient les informations sur les locations en cours
void updateRessourceLouerLocal(Modification_s *m, RessourceLoue_s *r){
    // On met à jour le tableau local des ressources loué
    // ATTENTION ! il faut que l'état du système ai bien modifier avec la demande du client avant de faire ce code
    if(m->type == 1){ // c'est une demande donc on la rajoute dans cette structure
        int siteDemande, modeDemande, cpuDemande;
        float stoDemande;
        
        for(int i=0;i<m->nbDemande;i++){
            //on recupère 
            siteDemande = m->tabDemande[i].idSite;
            modeDemande = m->tabDemande[i].mode;
            cpuDemande = m->tabDemande[i].cpu;
            stoDemande = m->tabDemande[i].sto;

            // si vrai ça veux dire qu'il qu'on a pas deja louer de ressource sur ce site avec ce mode
            if(r->tabLocation[siteDemande-1].idSite == -1){
                r->tabLocation[siteDemande-1].idSite = siteDemande;
                r->tabLocation[siteDemande-1].mode = modeDemande;
                r->tabLocation[siteDemande-1].cpu = cpuDemande;
                r->tabLocation[siteDemande-1].sto = stoDemande;
                r->nbRessources++;
            }else{
                // erreur, on a deja louer des ressources sur ce site on annule la demande
                perror("\nErreur : Tu as deja louer une ressource sur un site que tu demandes, libère là avant");
                exit(1);
            }
        }
    }else{ // alors on est dans une libération
        
    }
}

int main(int argc, char const *argv[]){
    int identifiantClient;
    if(argc != 3){
        printf("lancement : ./server chemin_fichier_cle idClient\n");
        exit(1);
    }
    if(atoi(argv[2])>200 || atoi(argv[2])<=0){
        printf("Erreur lancement : idClient doit être compris entre 1 et %d\n",NBMAXCLIENT);
        exit(1);
    }
    identifiantClient = atoi(argv[2]);

    

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
    semop(sem_id,opLock+1,1); // V sur le semaphore qui sert de lock pour l'état du système
    printf("La copy est terminé\n");

    // on initialise les ressources loué // on place tous les idSite à -1 pour dire qu'il ne sont pas louer
    // après on va le modifier pour initialiser se tableau avec l'état système, pour mettre ceux ou l'id de l'utilisateur est présent
    RessourceLoue_s ressourceLoue;
    ressourceLoue.nbRessources = 0;

    int nbMaxSite = NBSITES;
    for(int i=0;i<nbMaxSite;i++){
        ressourceLoue.tabLocation[i].idSite = -1;
    }



    //Etape 2 : creation du processus secondaire (thread) 
    //          qui se charge d'afficher l'état du système à chaque mise à jour

        // il faut la mettre à jour avec les modifications reçut
    /*- on affiche l'état seulement quand il y a une modification
      - detecté modification du sytème
      - mettre à jour la copie 
      - afficher l'état du système mis à jour */
    afficherEtatSysteme(&etatSystemCopyOnClient);

    // initialisation de la structure de donnée de demande de modification
    struct Modification_s modification;
    initStructModif(&modification);

    //Etape 3 :  Boucle qui attent les demandes de modifications (demande/libération)
    
        // on reset à chaque nouvelle demande de modification
        resetStructModif(&modification);

        // Demande à l'utilisateur si il veut demander des ressources ou en libérer
        messageChoixTypeAction();
        int typeAction = demandeTypeAction();
        if(typeAction == -1){
            printf("Erreur : Type d'action impossible\n");
            exit(1); 
        }else{
            modification.type = typeAction;
        }

        if(modification.type == 1){ // Pour une demande de ressource
            int response = 1;
             { // bloc commentaire
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
            }   
            do{
                demandeDeRessources(&modification);

                printf("\nEntre 1 si tu veux faire une autre demande (sinon met autre chose) : ");
                if(scanf("%d",&response) != 1 ){// si jamais un autre type que %d ou autre chose que 1 est entré
                    response = 0;
                }
                response = (response != 1 ? 0 : 1 );
            }while(response);
        }else{ 
            if(ressourceLoue.nbRessources == 0){ 
                // Si il n'y a pas encore de ressource loué, on ne peut pas en libérer
                printf("\nErreur : Impossible de liberer des ressources : Aucune ressource loué\n");
                exit(1);
            }else{
                afficherRessourcesLoue(&ressourceLoue);
                int site = demandeDeLiberations(&modification,&ressourceLoue);
                if(site == -1){
                    printf("Erreur : Libération impossible (le site n'est pas louer ou bien il n'existe pas)");
                    exit(1); 
                }
            }
        }
        afficherStructureRequete(&modification);
        printf("\n\n...Demande terminé\n\n");
        // On met a jour le tableau des ressources louer une fois que le server à accepté de changé l'état du système
        updateRessourceLouerLocal(&modification,&ressourceLoue);
    

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt -> détachement segment mémoire : état du système");
        exit(1);
    }

    printf("\n\n\nFin du Programme \n");

    return 0;
}