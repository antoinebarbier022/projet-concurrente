#include "struct.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

#define NBSITES 10 
#define NBMAXCLIENT 20

using namespace std;

void initStructModif(Modification_s *m, int idC){
    m->idClient = idC;
    m->type = 0;
    m->nbDemande = 0;
}
// on l'appel une fois une modification terminé
void resetStructModif(Modification_s *m, int idC){
    m->idClient = idC;
    m->type = 0;
    m->nbDemande = 0;
    for(int i=0;i<m->nbDemande;i++){
        m->tabDemande[i].idSite = -1;
        m->tabDemande[i].mode = 0;
        m->tabDemande[i].cpu = 0;
        m->tabDemande[i].sto = 0;
    }

}


// on récupère l'état système pour le mettre en local
void initEtatSystemeLocal(SystemState_s *s){

}

void updateEtatSystemeLocal(SystemState_s *s){

}

void initRessourcesLoue(SystemState_s *s, RessourceLoue_s *r, int idClient){
    for(int i=0;i<s->nbSites;i++){
        if(s->sites[i].tabUse[idClient-1].isUse){
            // si les sur un site dans le tableau d'utilisation à l'index idClient -1 on a noté comme loué alors l'utilisateur avec idClient à une location en cours
            r->nbRessources++;
            r->tabLocation[r->nbRessources-1].idSite = s->sites[i].id; // on place l'id du site dont on a louer des ressources
            r->tabLocation[r->nbRessources-1].mode = s->sites[i].tabUse[idClient-1].mode; // Mode
            r->tabLocation[r->nbRessources-1].cpu = s->sites[i].tabUse[idClient-1].cpu;
            r->tabLocation[r->nbRessources-1].sto = s->sites[i].tabUse[idClient-1].sto;
        }else{

        }
    }
}

// etatSystemeLocal est la copie de l'état du système du server, 
void afficherEtatSysteme(SystemState_s *etatSystemeLocal){
    printf("\033[36m "); // couleur du texte
    printf("\nEtat du système :\n");
    for(int i=0;i<etatSystemeLocal->nbSites;i++){
        printf("  - [Site %d] : %d cpu free, %.1f Go free \n",   
                etatSystemeLocal->sites[i].id, 
                etatSystemeLocal->sites[i].cpuFree,
                etatSystemeLocal->sites[i].stoFree);
    }
    printf("\033[m");
}

void afficherStructureRequete(Modification_s *m){
    printf("\033[32m "); // couleur du texte
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
    printf("\033[m");
    printf("\n");
}

void afficherRessourcesLoue(RessourceLoue_s *r){
    printf("\033[35m "); // couleur du texte
    printf("\nRessource loués :");
    if(r->nbRessources != 0){
        int nbSite = NBSITES;
        for(int i=0;i<nbSite;i++){
            if(r->tabLocation[i].idSite != -1){
                printf("\n  - [Site %d] : %d cpu, %.1f Go ",  
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
    printf("\033[m");
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
    char type[10];
    // Choix de l'action à effectué (demande de ressource ou bien libération)
    printf("\nQuelle action voulez-vous effectuer ? (1 ou 2) : ");
    cin.getline(type,10);
    if(type[0] == '2' || type[0] == '1'){
        return atoi(type);
    }else{
        return -1;
    }
}

// Cette fonction demande les ressources à louer au client et les places dans la stucture Modification
void demandeDeRessources(Modification_s *m){
    int inputId, inputMode, inputCpu;
    float inputSto;
    printf("\nLa demande doit correspondre à ce format : idSite mode nbCPU nbSto\n");
    printf("Demande de ressource : ");
    cin >> inputId >> inputMode >> inputCpu >> inputSto;
    if(!cin){
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
        if(m->nbDemande >= NBMAXCLIENT){
            perror("erreur : le nombre de demande ne peux pas être supérieur au nombre de site.\n");
            exit(1); 
        }
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
    printf("\nSur quelle site veux-tu libérer les ressources ? \nSite : ");
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

void traitementLiberation(SystemState_s* s,RessourceLoue_s *r, int idClient, int idSiteLiberer){
    int mode = s->sites[idSiteLiberer-1].tabUse[idClient-1].mode;
    int cpu = s->sites[idSiteLiberer-1].tabUse[idClient-1].cpu;
    float sto = s->sites[idSiteLiberer-1].tabUse[idClient-1].sto;

    s->sites[idSiteLiberer-1].stoFree += sto; // on remet le stockage qu'on avait loué
    if(mode == 1){ // exclusif
        s->sites[idSiteLiberer-1].cpuExclusif -= cpu; // on enleve le nombre de cpu loué
        s->sites[idSiteLiberer-1].cpuFree += cpu; // on rajoute nos cpu en dispo
    }else{
        // regarder si le nombre cpu loue est le max
        // si oui alors on cherche le 2 eme maximum
        if(cpu == s->sites[idSiteLiberer-1].maxCpuPartage){
            // on doit trouvé le deuxième cpu partagé max pour le faire devenir le max
            int newCpuMax = 0;
            for(int i = 0;i<NBMAXCLIENT;i++){
                if(i != idClient-1){ // On regarde qu'on ne compare pas avec lui même
                    if(s->sites[idSiteLiberer-1].tabUse[i].isUse){ // il faut qu'un client ai louer des cpu
                        if(s->sites[idSiteLiberer-1].tabUse[i].mode == 2){ // en mode partagé
                            if(s->sites[idSiteLiberer-1].tabUse[i].cpu > newCpuMax){ // 
                                newCpuMax = s->sites[idSiteLiberer-1].tabUse[i].cpu ;
                            }
                        }
                    }
                }
            }
            s->sites[idSiteLiberer-1].cpuFree += (s->sites[idSiteLiberer-1].maxCpuPartage - newCpuMax);
            s->sites[idSiteLiberer-1].maxCpuPartage = newCpuMax;
        }
    }
    //on remet à 0 le tableau de la structure de donnée de la location du client sur ce site
    s->sites[idSiteLiberer-1].tabUse[idClient-1] = {0, 0, 0, 0};
}
// verification si c'est possible de mettre à jour le système avec la demande
// retour :
//   - 1 = les ressources sont dispo pour la demande
//   - 2 =  attente car les ressources ne sont pas dispo pour la demande
//   - -1 = erreur, la demande est impossible dans tous les cas
int checkDemandeValide(SystemState_s* s, Modification_s *m){
    int i = 0;
    while(i < m->nbDemande){
        // test la validité du site
        int idSiteDemande = m->tabDemande[i].idSite;
        if(idSiteDemande <= s->nbSites && idSiteDemande > 0){
            if(m->tabDemande[i].cpu < s->sites[idSiteDemande - 1].cpu && m->tabDemande[i].cpu > 0){
                if(m->tabDemande[i].sto < s->sites[idSiteDemande - 1].sto && m->tabDemande[i].sto > 0){
                    return 1;
                }else{
                    return -1;
                }
            }else{
                return -1; // impossible car le nombre de cpu ne pourra jamais être offert
            }
        }else{
            perror("\nID non valide\n");
            return -1; // impossible de traiter la demande car l'id du site est invalide
        }
        i++;
    }
    return 1;
}
int checkRessourcesDispo(SystemState_s* s, Modification_s *m){
    int i = 0;
    while(i < m->nbDemande){
        // test de la disponibilité des ressources
        int nbStoDemande = m->tabDemande[i].sto; 
        int modeDemande = m->tabDemande[i].mode; 
        int nbCpuDemande = m->tabDemande[i].cpu; 

        int idSiteDemande = m->tabDemande[i].idSite;
        int nbCpuExclusifRestant = s->sites[idSiteDemande-1].cpu - (s->sites[idSiteDemande-1].cpuExclusif + s->sites[idSiteDemande-1].maxCpuPartage);
        int nbCpuPartageRestant = nbCpuExclusifRestant + s->sites[idSiteDemande-1].maxCpuPartage;
        int nbStoRestant = s->sites[idSiteDemande-1].stoFree;

        // d'abord on regarde pour le stockage
        if((nbStoRestant - nbStoDemande) < 0){
            perror("Sto insuffisant");
            return -1; // nb stockage insuffisant
        }
        // ensuite on regarde pour le cpu
        if(modeDemande == 1){ //exclusif
            if((nbCpuExclusifRestant - nbCpuDemande) < 0){
                perror("cpuEx insuffisant");
                return -1; // nb ressources inssufisant
            }
        }else{ // partagé
            if((nbCpuPartageRestant - nbCpuDemande) < 0){
                perror("cpuSh insuffisant");
                return -1; // nb ressources inssufisant
            }
        }
        i++;
    }
    return 1; // Si on arrive ici c'est qu'il n'y a pas de problème, toutes les ressources sont dispo
}

// on applique la demande sur le segment mémoire
void traitementDemande(SystemState_s* s, Modification_s *m){
    //printf("\n Traintement en cours\n");
    int i = 0;
    int idClient = m->idClient;
    while(i < m->nbDemande){
        int idSiteDemande = m->tabDemande[i].idSite;
        float nbStoDemande = m->tabDemande[i].sto; 
        int modeDemande = m->tabDemande[i].mode; 
        int nbCpuDemande = m->tabDemande[i].cpu; 
        //printf("\n Demande sur le site %d -> %f sto, mode : %d, %d cpu \n",idSiteDemande, nbStoDemande, modeDemande, nbCpuDemande);

        s->sites[idSiteDemande-1].stoFree -= nbStoDemande;
        if(modeDemande == 1){ // exclusif
            s->sites[idSiteDemande-1].cpuExclusif += nbCpuDemande;
            s->sites[idSiteDemande-1].cpuFree -= nbCpuDemande;
        }else{
            if(s->sites[idSiteDemande-1].maxCpuPartage < nbCpuDemande){
                s->sites[idSiteDemande-1].cpuFree -= nbCpuDemande - s->sites[idSiteDemande-1].maxCpuPartage;
                s->sites[idSiteDemande-1].maxCpuPartage = nbCpuDemande;
            }
        }
        // on complète le tableau d'utilisation sur le site pour le client
        s->sites[idSiteDemande-1].tabUse[idClient - 1].isUse = 1;
        s->sites[idSiteDemande-1].tabUse[idClient - 1].mode = modeDemande;
        s->sites[idSiteDemande-1].tabUse[idClient - 1].cpu = nbCpuDemande;
        s->sites[idSiteDemande-1].tabUse[idClient - 1].sto = nbStoDemande;
        
        //printf("\n Le client %d à loué %d cpu en mode : %d et %f Go sur le site %d ->  \n",idClient,nbCpuDemande,modeDemande,nbStoDemande, idSiteDemande);

        i++;
    }
    //printf("\n Fin du Traintement\n");
    return ;
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
    printf("\e[1;1H\e[2J");// efface la console
    int i, j, n;
    for (i = 0; i < 11; i++) {
        for (j = 0; j < 10; j++) {
        n = 10*i + j;
        if (n > 108) break;
        printf("\033[%dm %3d\033[m", n, n);
        }
        printf("\n");
    }

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
    key_t cle = ftok(argv[1], 'z');         // clé du segment mémoire pour l'état du système
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
    struct SystemState_s* p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat -> attachemnt segment mémoire : état du système");
        exit(1);
    }

    // on initialise les ressources loué // on place tous les idSite à -1 pour dire qu'il ne sont pas louer
    // après on va le modifier pour initialiser se tableau avec l'état système, pour mettre ceux ou l'id de l'utilisateur est présent
    RessourceLoue_s ressourceLoue;
    ressourceLoue.nbRessources = 0;

    int nbMaxSite = NBSITES;
    for(int i=0;i<nbMaxSite;i++){
        ressourceLoue.tabLocation[i].idSite = -1;
    }

    //Etape 1 : créer une copie local du système

    // copie de l'état système sur le client
    printf("Attente de la copie du système\n");
    semop(sem_id,opLock,1); // P sur le semaphore qui sert de lock pour l'état du système
        struct SystemState_s etatSystemCopyOnClient = *p_att;
        printf("... Copy en cours ...\n");
        sleep(1);
        // on initialise le tableau de ressources qui sont loué par l'utilisateur
        initRessourcesLoue(p_att, &ressourceLoue, identifiantClient);

    semop(sem_id,opLock+1,1); // V sur le semaphore qui sert de lock pour l'état du système
    printf("La copy est terminé\n");

    afficherRessourcesLoue(&ressourceLoue);


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
    initStructModif(&modification, identifiantClient);

    //Etape 3 :  Boucle qui attent les demandes de modifications (demande/libération)
    
        // on reset à chaque nouvelle demande de modification
        resetStructModif(&modification, identifiantClient);

        // Demande à l'utilisateur si il veut demander des ressources ou en libérer
        messageChoixTypeAction();
        int typeAction = demandeTypeAction();
        if(typeAction == -1){
            printf("Erreur : Type d'action impossible\n");
            exit(1); 
        }else{
            modification.type = typeAction;
        }
        int siteALiberer;
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
                siteALiberer = demandeDeLiberations(&modification,&ressourceLoue);
                if(siteALiberer == -1){
                    printf("Erreur : Libération impossible (le site n'est pas louer ou bien il n'existe pas)");
                    exit(1); 
                }else{
                    printf("On va demandé la libération du site %d",siteALiberer);
                    
                }
            }
        }

        printf("\e[1;1H\e[2J");// efface la console
        //afficherEtatSysteme(&etatSystemCopyOnClient);
        afficherStructureRequete(&modification);

        int demandeAccepte = 0; // vrai une fois le traittement fait
        //lock
        semop(sem_id,opLock,1); // P sur le semaphore qui sert de lock pour l'état du système
            if(typeAction == 2){
                traitementLiberation(p_att,&ressourceLoue, identifiantClient, siteALiberer);
            }else{
                // On regarde d'abord si la demande est valide
                if(checkDemandeValide(p_att, &modification) == -1){
                    printf("Erreur : La demande est impossible car l'id du site n'est pas valide ou alors le nombre de ressouce demandé ne pourra jamais être disponible\n ");
                    exit(1); 
                }else{
                    if(checkRessourcesDispo(p_att, &modification) != -1){
                        traitementDemande(p_att, &modification);
                        demandeAccepte = 1;
                    }else{
                        // il va falloir re essayer quand il y aura une notification et une libération dans le systeme
                        printf("\n On est en attente de dispo de ressource\n");
                    }
                }
            }
            

            afficherEtatSysteme(p_att); // afficher le distant
        //unlock
        semop(sem_id,opLock+1,1); // V sur le semaphore qui sert de lock pour l'état du système

        if(demandeAccepte){
            // On met a jour le tableau des ressources louer une fois que le server à accepté de changé l'état du système
            updateRessourceLouerLocal(&modification,&ressourceLoue);
        }

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("erreur : shmdt -> détachement segment mémoire : état du système");
        exit(1);
    }

    printf("\n\n\nFin du Programme \n");

    return 0;
}