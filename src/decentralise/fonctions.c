#include "struct.h"
#include "fonctions.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <iostream>

using namespace std;


int initSysteme(SystemState_s *s,const char* nomFichier){

    FILE* fichier = NULL;
    fichier = fopen(nomFichier,"r"); // ouverture du fichier en lecture seul
    if(fichier == NULL){
        printf(BRED "Erreur : Impossible d'ouvrir le fichier d'initialisation des sites\n" reset);
        return -1;
    }
    s->nbClientConnecte=0;
    char name[30];
    int id, cpu, sto;
    id = 0;
    int nbSite = 0;
    while(!feof(fichier)){
        if(fscanf(fichier, "%s : %d %d\n",name, &cpu, &sto) != 3){
            printf(BRED "Erreur : Le fichier d'initialisation [site.txt] est mal formé\n" reset);
            return -1;
        }
        if(cpu > 500){
            printf(BRED "Erreur : [fichier sites.txt] Le nombre de cpu sur un site ne peut pas être supérieur à 500\n" reset);
            return -1;
        }
        nbSite++;
    }
    rewind(fichier); // on replace le curseur au début du fichier

    s->nbSites = nbSite;
    char buffer[30] = {'\0'};
    for(int i=0;i<NBCLIENTMAX;i++){
        strcpy(s->tabId[i],buffer);
        s->tabNotif[i] = -1;
    }


    while(!feof(fichier)){
        fscanf(fichier, "%s : %d %d\n",name, &cpu, &sto); // on a deja regarder si il y avait une erreur donc pas de vérification à faire
        id++;

        s->sites[id-1].id = id;
        strcpy(s->sites[id-1].nom, name);
        s->sites[id-1].cpu = cpu;
        s->sites[id-1].sto = sto;
        s->sites[id-1].resCpuExFree = (float)cpu;
        s->sites[id-1].resCpuShFree = (float)cpu*CPUSHARED;
        s->sites[id-1].resStoExFree = (float)sto;
        s->sites[id-1].resStoShFree = (float)sto*CPUSHARED;
        s->sites[id-1].nbMaxClientEx = cpu;
        s->sites[id-1].nbMaxClientSh = CPUSHARED*cpu; // CPUSHARED = le nombre de partage possible d'un cpu
    }    

    //vérifier si le nom d'un site n'a pas été entrée plus d'une fois
    for(int i=0;i<s->nbSites;i++){
        for(int x=0;x<s->nbSites;x++){
            if(i != x && strcmp(s->sites[i].nom,s->sites[x].nom) == 0){
                printf(BRED "Erreur : [fichier sites.txt] Impossible d'avoir deux sites avec le même nom.\n" reset);
                return -1;
            }
        }
    }
    fclose(fichier);

    return 1;
}

void afficherSysteme(SystemState_s *s){
    printf(BCYN"\n         Etat du système : Ressources disponible" reset );
    printf(BCYN"\n┏━━━━━━┳━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━┓"  );
    printf(BCYN"\n┃      ┃       Nom       ┃      Exclusif        ┃          Partagé         ┃"  );
       printf( "\n┃  id  ┃   Nom du site   ┃   cpu        sto     ┃     cpu         sto      ┃  " reset);
    printf(BCYN"\n┣━━━━━━╋━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━━━━━━━━━━━┫"  );
    printf("\n" CYN);
    for(int i=0;i<s->nbSites;i++){
        printf("┃ %-4d ┃ %-15s ┃ %3.0f/%-3d  %5.0f/%-5d ┃ %5.0f/%-4d  %5.0f/%-6d ┃\n",   
                s->sites[i].id,
                s->sites[i].nom, 
                s->sites[i].resCpuExFree,
                s->sites[i].cpu,
                s->sites[i].resStoExFree,
                s->sites[i].sto,
                s->sites[i].resCpuShFree,
                s->sites[i].cpu*CPUSHARED, // multiplier par le nombre de partage d'un cpu
                s->sites[i].resStoShFree,
                s->sites[i].sto*CPUSHARED);
    }
    printf(BCYN"┗━━━━━━┻━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━━━┛"  );
    printf("\n" reset); // reset color
}

void afficherRessourcesLoue(Requete_s *ressourceLoue){
    printf(BHGRN "\nRessource loués :");
    if(ressourceLoue->nbDemande != 0){
        int i = 0;
        int cmpt = ressourceLoue->nbDemande;
        while(cmpt> 0 && i<NBSITEMAX){
            if(ressourceLoue->tabDemande[i].idSite != 0){
                printf("\n  - [Site %d] : %d cpu, %d Go ",  
                    ressourceLoue->tabDemande[i].idSite, 
                    ressourceLoue->tabDemande[i].cpu, 
                    ressourceLoue->tabDemande[i].sto);
                if(ressourceLoue->tabDemande[i].mode == 1){
                    printf("[mode exclusif]");
                }else{
                    printf("[mode partagé]");
                }
                --cmpt;
            }
            i++;
        }
    }else{
        printf(" Aucune");
    }
    printf("\n" reset);
}

void updateSysteme(SystemState_s *s, Requete_s *r){
    
    if(r->type == 2){ // liberation
        for(int i=0;i<r->nbDemande;i++){
            if(r->tabDemande[i].mode == 1){ // exclusif
                s->sites[r->tabDemande[i].idSite-1].resCpuExFree += r->tabDemande[i].cpu; // mettre valeur 
                s->sites[r->tabDemande[i].idSite-1].resStoExFree += r->tabDemande[i].sto;
                s->sites[r->tabDemande[i].idSite-1].resCpuShFree += r->tabDemande[i].cpu*CPUSHARED;
                s->sites[r->tabDemande[i].idSite-1].resStoShFree += r->tabDemande[i].sto*CPUSHARED;

                s->sites[r->tabDemande[i].idSite-1].nbMaxClientEx--;

                int n = 0;
                while(n <= s->sites[r->tabDemande[i].idSite-1].nbMaxClientEx){
                    if(s->sites[r->tabDemande[i].idSite-1].tabUseExclusif[n].idClient == r->idClient){ // pas de client ici
                        s->sites[r->tabDemande[i].idSite-1].tabUseExclusif[n] = {0, -1, 0, 0};
                    }
                    n++;
                }

            }else{ // partagé
                s->sites[r->tabDemande[i].idSite-1].resCpuExFree += (float)r->tabDemande[i].cpu/CPUSHARED; // mettre valeur 
                s->sites[r->tabDemande[i].idSite-1].resStoExFree += (float)r->tabDemande[i].sto/CPUSHARED;
                s->sites[r->tabDemande[i].idSite-1].resCpuShFree += r->tabDemande[i].cpu;
                s->sites[r->tabDemande[i].idSite-1].resStoShFree += r->tabDemande[i].sto;
            
                s->sites[r->tabDemande[i].idSite-1].nbMaxClientSh--;

                int n = 0;
                while(n <= s->sites[r->tabDemande[i].idSite-1].nbMaxClientSh){
                    if(s->sites[r->tabDemande[i].idSite-1].tabUseShare[n].idClient == r->idClient){ // pas de client ici
                        s->sites[r->tabDemande[i].idSite-1].tabUseShare[n] = {0, -1, 0, 0};
                    }
                    n++;
                }
            }
        }
    }
    else{ // demande ressource
        for(int i=0;i<r->nbDemande;i++){
            if(r->tabDemande[i].mode == 1){ // exclusif
                s->sites[r->tabDemande[i].idSite-1].resCpuExFree -= r->tabDemande[i].cpu; // mettre valeur 
                s->sites[r->tabDemande[i].idSite-1].resStoExFree -= r->tabDemande[i].sto;
                s->sites[r->tabDemande[i].idSite-1].resCpuShFree -= r->tabDemande[i].cpu*CPUSHARED;
                s->sites[r->tabDemande[i].idSite-1].resStoShFree -= r->tabDemande[i].sto*CPUSHARED;

                s->sites[r->tabDemande[i].idSite-1].nbMaxClientEx++;
                int n = 0;
                while(n <= s->sites[r->tabDemande[i].idSite-1].nbMaxClientEx){
                    if(s->sites[r->tabDemande[i].idSite-1].tabUseExclusif[n].isUse == 0){ // pas de client ici
                        s->sites[r->tabDemande[i].idSite-1].tabUseExclusif[n] = {1, r->idClient, r->tabDemande[i].cpu, r->tabDemande[i].sto};
                    }
                    n++;
                }
                

            }else{ // partagé
                s->sites[r->tabDemande[i].idSite-1].resCpuExFree -= (float)r->tabDemande[i].cpu/CPUSHARED; // mettre valeur 
                s->sites[r->tabDemande[i].idSite-1].resStoExFree -= (float)r->tabDemande[i].sto/CPUSHARED;
                s->sites[r->tabDemande[i].idSite-1].resCpuShFree -=  r->tabDemande[i].cpu;
                s->sites[r->tabDemande[i].idSite-1].resStoShFree -= r->tabDemande[i].sto;
            
                s->sites[r->tabDemande[i].idSite-1].nbMaxClientSh++;
                int n = 0;
                while(n <= s->sites[r->tabDemande[i].idSite-1].nbMaxClientSh){
                    if(s->sites[r->tabDemande[i].idSite-1].tabUseShare[n].isUse == 0){ // pas de client ici
                        s->sites[r->tabDemande[i].idSite-1].tabUseShare[n] = {1, r->idClient, r->tabDemande[i].cpu, r->tabDemande[i].sto};
                    }
                    n++;
                }
            }
            
        }
    }


    

}

void updateRessourceLoueLocal(Requete_s *r, Requete_s *ressourceLoue){
    //enregistrer le contenue de la requete
    for(int i=0;i<r->nbDemande;i++){
        int trouve = 0, y = 0;
        if(r->type==1){ // si c'est une demande on insere
            while(!trouve && y<NBSITEMAX){
                // dès qu'on trouve une place dans le tableau, on insère
                if(ressourceLoue->tabDemande[y].idSite == -1){ // on a obligatoirement du cpu
                    /*printf("site %d : %d cpu, %d sto, mode : %d\n",
                    r->tabDemande[i].idSite,
                    r->tabDemande[i].cpu,
                    r->tabDemande[i].sto,
                    r->tabDemande[i].mode);*/
                    ressourceLoue->tabDemande[y] = r->tabDemande[i];
                    ressourceLoue->nbDemande++;
                    
                    trouve = 1; // on a trouver une place
                }
                
                y++;
            }
        }else{
            while(!trouve && y<NBSITEMAX){
                // dès qu'on trouve le site dans le tableau, on le supprime
                if(ressourceLoue->tabDemande[y].idSite == r->tabDemande[i].idSite){
                    ressourceLoue->tabDemande[y] = {0,0,0,0};
                    ressourceLoue->nbDemande--;
                    trouve = 1; // on a trouver une place
                }
                y++;
            }
        }
    }
}

void lockSysteme(int sem_id){
    struct sembuf op = {(u_short)0,(short)-1,SEM_UNDO};
    semop(sem_id,&op,1);
}

void unlockSysteme(int sem_id){
    struct sembuf op = {(u_short)0,(short)+1,SEM_UNDO};
    semop(sem_id,&op,1);
}

int attribuerIdentifiantClient(SystemState_s *s, const char* nom){
    int id = 0;
    char buffer[30] = {'\0'};
    while(strcmp(s->tabId[id],buffer) && id < NBCLIENTMAX){
        id++;
    }
    strcpy(s->tabId[id], nom);
    return id+1; // l'id sera sur l'index id-1 du tableau
}

int attribuerNumSemNotification(SystemState_s *s, int id){
    int i = 1; // on commence à 1 car le 0 est reservé au parent pour le test
    int numSem = -1;
    while(s->tabNotif[i] != -1 && i < 2000){
        i++;
    }
    s->tabNotif[i] = id;
    return i;
}

int saisieTypeAction(){
    int type = 0;
    printf("\nType action possible : ");
    printf("\n - 1 : Demande de réservation");
    printf("\n - 2 : Demande de libération");
    printf("\n - 3 : Quitter\n");
    do{
        printf("Type action : ");
        cin >> type;
        if(!cin){
            printf(BRED "Erreur : La saisie est incorrect.\n" reset);
            type = 3; // quitter
        }
        if(type != 1 && type != 2 && type != 3){
            printf(BRED "Erreur : La saisie est incorrect.\n" reset);
        }
    }while(type != 1 && type != 2 && type != 3);

    return type;
    
}

int saisieDemandeRessource(Requete_s *r, Requete_s *ressourceLoue, SystemState_s *s){
    int inputId, inputMode, inputCpu, inputSto;
    printf("\n\nLa demande doit correspondre à ce format :");
    printf(BHWHT" idSite mode cpu stockage\n" reset);
    printf("Pour le mode : [1 = exclusif] et [2 = partagé]\n");
    int erreur;
    do{
        erreur = 0; // il n'y a pas d'erreur
        printf("\nDemande de ressource : ");
        cin >> inputId >> inputMode >> inputCpu >> inputSto;
        //printf("\n");
        if(!cin){
            // Si le format n'est pas correct on indique la bonne utilisation à l'utilisateur on on quitte le programme
            printf(BRED "Erreur : La demande doit correspondre à ce format : id mode cpu sto\n");
                printf("\tid   : l'identifiant du site \n");
                printf("\tmode : Exclusif (1) ou Partagé (2)\n");
                printf("\tcpu  : Le nombre de CPU demandé \n");
                printf("\tsto  : Le nombre de Go de stockage demandé \n\n");
                printf(reset);
            return -1;
        }else{ // le format est correct mais cela ne veut pas dire que la demande l'est.
            //printf("[Demande de ressource] Site %d : %d cpu, %.1f Go en mode %d\n", inputId, inputCpu, inputSto, inputMode);
            // Sinon on entre la demande

            if(ressourceLoue->nbDemande > 0){
                int i=0;
                while(i<NBSITEMAX){
                    while(inputId == ressourceLoue->tabDemande[i].idSite){
                        printf(BRED "Erreur : Vous avez déja reserver des ressources sur ce site. Il faut libérer les ressources reservé avant de faire une nouvelle demande.\n" reset);
                        erreur = 1;
                    }
                    i++;
                }
            }
            // mode incorect
            if(!(inputMode == 2 || inputMode == 1)){
                printf(BRED "Erreur : le mode doit être 1 pour le <mode exclusif> ou 2 pour le <mode partagé>\n" reset);
                erreur = 1;
            }

            // test la validité du site
            if(inputId <= s->nbSites && inputId > 0){
                if(inputMode == 1){ // en exclusif
                    if(inputCpu <= s->sites[inputId-1].cpu && inputCpu >= 0){
                        if(inputSto < s->sites[inputId-1].sto && inputSto >= 0){
                            //ok
                        }else{
                            printf(BRED "Erreur : nombre de Go de stockage non valide\n" reset);
                            erreur = 1;
                        }
                    }else{
                        printf(BRED "Erreur : nombre de CPU non valide\n" reset);
                        erreur = 1; // impossible car le nombre de cpu ne pourra jamais être offert
                    }
                }else{ // en partagé
                    if(inputCpu <= s->sites[inputId- 1].cpu*CPUSHARED && inputCpu >= 0){
                        if(inputSto < s->sites[inputId- 1].sto*CPUSHARED && inputSto >= 0){
                            //ok
                        }else{
                            printf(BRED "Erreur : nombre de Go de stockage non valide\n" reset);
                            erreur = 1;
                        }
                    }else{
                        printf(BRED "Erreur : nombre de CPU non valide\n" reset);
                        erreur = 1; // impossible car le nombre de cpu ne pourra jamais être offert
                    }
                }
            }else{
                printf(BRED "Erreur : ID du site non valide\n" reset);
                erreur = 1; // impossible de traiter la demande car l'id du site est invalide
            }
        }
    }while(erreur);
    
    // on place les données demandé dans la structure
    r->tabDemande[r->nbDemande] = {inputId, inputMode, inputCpu, inputSto};
    r->nbDemande++;
    return 1;
    
}

int saisieDemandeLiberation(Requete_s *r, Requete_s *ressourceLoue){
    int inputIdSite;
    printf("\n\nSur quelle site veux-tu libérer les ressources ? \nSite : ");
    cin >> inputIdSite;
    if(!cin){
        printf(BRED "Erreur : entrée non attendu" reset);
        return -1;
    }else{
        int i = 0;
        int trouve = 0;
        while(!trouve && i<NBSITEMAX){
            if(ressourceLoue->tabDemande[i].idSite == inputIdSite){
                // on peut libérer cette ressource car elle a été loué
                r->nbDemande++;
                r->tabDemande[r->nbDemande-1] = {
                    ressourceLoue->tabDemande[i].idSite, 
                    ressourceLoue->tabDemande[i].mode, 
                    ressourceLoue->tabDemande[i].cpu, 
                    ressourceLoue->tabDemande[i].sto};
                    trouve = 1;
            }
            i++;
        }
    }
    return 1;
}

int demandeRessourceValide(SystemState_s *s, Requete_s *r){
   int i = 0;
    while(i < r->nbDemande){
        int idSiteDemande = r->tabDemande[i].idSite;

        //test : si le nombre de cpu est > 0 : demande impossible sinon
        if(r->tabDemande[i].cpu <= 0){
            printf(BRED "\nErreur : demande impossible car le nombre de cpu doit être supérieur à 0\n" reset);
            return -1;
        }
        // test la validité du site
        if(idSiteDemande <= s->nbSites && idSiteDemande > 0){
            if(r->tabDemande[i].mode == 1){ // en exclusif
                if(r->tabDemande[i].cpu <= s->sites[idSiteDemande - 1].cpu && r->tabDemande[i].cpu >= 0){
                    if(r->tabDemande[i].sto < s->sites[idSiteDemande - 1].sto && r->tabDemande[i].sto >= 0){
                        return 1;
                    }else{
                        printf(BRED "\nErreur : nombre de Go de stockage non valide\n" reset);
                        return -1;
                    }
                }else{
                    printf(BRED "\nErreur : nombre de CPU non valide\n" reset);
                    return -1; // impossible car le nombre de cpu ne pourra jamais être offert
                }
            }else{ // en partagé
                if(r->tabDemande[i].cpu <= s->sites[idSiteDemande - 1].cpu*CPUSHARED && r->tabDemande[i].cpu >= 0){
                    if(r->tabDemande[i].sto < s->sites[idSiteDemande - 1].sto*CPUSHARED && r->tabDemande[i].sto >= 0){
                        return 1;
                    }else{
                        printf(BRED "\nErreur : nombre de Go de stockage non valide\n" reset);
                        return -1;
                    }
                }else{
                    printf(BRED "\nErreur : nombre de CPU non valide\n" reset);
                    return -1; // impossible car le nombre de cpu ne pourra jamais être offert
                }
            }
        }else{
            printf(BRED "\nErreur : ID du site non valide\n" reset);
            return -1; // impossible de traiter la demande car l'id du site est invalide
        }
        i++;
    }
    return 1;
}

int demandeLiberationValide(SystemState_s *s, Requete_s *r){
   int idSiteDemande;
   int i = 0;
   while(i < r->nbDemande){ 
       idSiteDemande = r->tabDemande[i].idSite;
       if(idSiteDemande > s->nbSites && idSiteDemande <= 0){
           return -1;
       }
       i++;
   }
   return 1;
}

int demandeValide(SystemState_s *s, Requete_s *r){
    if(r->type == 1){
        if(demandeRessourceValide(s,r) == -1){
            printf(BRED "Erreur : La demande de ressources n'est pas valide.\n" reset);
            return -1;
        }else{
            return 1;
        }
        
    }else{
        if(demandeLiberationValide(s,r) == -1){
            printf(BRED "Erreur : La demande de libération n'est pas valide.\n" reset);
            return -1;
        }else{
            return 1;
        }
    }
}

int initTableauSemSites(int sem_id, SystemState_s *s){
    union semun{
        int val;                // cmd= SETVAL
        struct semid_ds *buf;   // cmd= IPC_STAT ou IPC_SET
        unsigned short *array;  // cmd= GETALL ou SETALL
        struct seminfo *__buf;  // cmd= IPC_INFO (sous linux)
    }egCtrl;

    int nbSemaphore = CPUSHARED*s->nbSites+1;
    // premier semaphore = verou seg memoire
    egCtrl.val =1;
    if(semctl(sem_id, 0, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
        perror(BRED "Erreur : initialisation [sémaphore 0]" reset);
        return -1;
    }
    int numSem;
    for(int i=1;i<=s->nbSites;i++){
        numSem = i*CPUSHARED;
        for(int x=3;x>=0;x--){
            numSem = i*CPUSHARED-x;
            switch (x){
                case 0: egCtrl.val = (s->sites[i-1].sto*4)*CPUSHARED; // sto shared
                    break;
                case 1: egCtrl.val = (s->sites[i-1].sto*4); // sto exclusif
                    break;
                case 2: egCtrl.val = (s->sites[i-1].cpu*4)*CPUSHARED; // cpu shared
                    break;
                case 3: egCtrl.val = (s->sites[i-1].cpu*4); // cpu ex
                    break;
            }
            if(semctl(sem_id, numSem, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
                printf(BRED "Erreur : Initialisation [sémaphore %d]\n",numSem, reset);
                return -1;
            }
        }
    }
    return 1;
}

int initOp(struct sembuf *op, Requete_s *r){
    int t = (r->type == 1 ? -1 : 1); // soit -1 pour une demande soit 1 pour une liberation de reservation
    // Buffer Demande
    Demande_s buffer;
    int numOp = 0;
    short valeur;
    // faire un x*operateur[0] est equivalant à x
    // faire un x*operateur[1] est equivalant à x*4
    // faire un x*operateur[2] est equivalant à x/4
    for(int i=0;i<r->nbDemande;i++){
        
        // on a tout multiplier par CPUSHARED pour ne pas avoir de nombre à virgule quand on fait une division (pas de float avec semaphores)
        buffer = { r->tabDemande[i].idSite, r->tabDemande[i].mode, r->tabDemande[i].cpu*CPUSHARED, r->tabDemande[i].sto*CPUSHARED};
        
        for(int x=3;x>=0;--x){
            int numSem = (buffer.idSite*4-x); // position du semaphore pour le site
            // si on demande de l'exclusif ou partagé
            float operateur[2] = {0,0};
            operateur[0] = (buffer.mode == 1 ? 1 : 0.25) ;
            operateur[1] = (buffer.mode == 1 ? CPUSHARED : 1) ;
            switch (x){
                case  0: // modification semaphore stockage partagé du site i
                    // pour une demande de Stockage en exclusif :    valeur = t*(sto*4)*CPUSHARED
                    // pour une demande de Stockage en partagé :     valeur = t*(sto*4)
                    valeur = (t*buffer.sto*operateur[1]);
                    break;
                case  1: // modification semaphore stockage  exclusif du site i
                    // pour une demande de Stockage en exclusif :    valeur = t*(sto*4)
                    // pour une demande de Stockage en partagé :     valeur = t*(sto*4)/CPUSHARED
                    valeur = (t*buffer.sto*operateur[0]);
                    break;
                case  2: // modification semaphore Cpu partagé du site i
                    valeur = (t*buffer.cpu*operateur[1]);
                    // pour une demande de CPU en exclusif :    valeur = t*(sto*4)*CPUSHARED
                    // pour une demande de CPU en partagé :     valeur = t*(sto*4)
                    break;
                case  3: // // modification semaphore Cpu Exclusif du site i
                    // pour une demande de CPU en exclusif :    valeur = t*(sto*4)
                    // pour une demande de CPU en partagé :     valeur = t*(sto*4)/CPUSHARED
                    valeur = (t*buffer.cpu*operateur[0]);
                    break;
                default:
                    break;
            }
            if(valeur != 0){ // car si valeur = 0 on a pas à faire d'opération
                op[numOp++] = {(u_short)numSem,(short)valeur,SEM_UNDO};
                //printf("sem:%d -> valeur :  %d\n",numSem,valeur);
            }
        }
    }
    return numOp;
}

int emmetreNotif(int idTabSem){
    union semun{
        int val;                // cmd= SETVAL
        struct semid_ds *buf;   // cmd= IPC_STAT ou IPC_SET
        unsigned short *array;  // cmd= GETALL ou SETALL
        struct seminfo *__buf;  // cmd= IPC_INFO (sous linux)
    }egCtrl;
    egCtrl.val =1;
    for(int i=0;i<NBCLIENTMAX;i++){
        if(semctl(idTabSem, i, SETVAL, egCtrl) == -1){  // On SETVAL pour le sémaphore numero 0
            perror(BRED "Erreur : initialisation [sémaphore 0]" reset);
            return -1;
        }
    }
    return 1;
}

void fermerClient(int idClient, int numSemNotif, struct SystemState_s *p_att, Requete_s *ressourceLoue, int shm_id, int sem_id, int sem_idNotif){
    // libération des ressources
    lockSysteme(sem_id);
        p_att->tabNotif[numSemNotif] = -1; // supprimer le semaphore de l'utilisateur
        char buffer[30] = {'\0'};
        strcpy(p_att->tabId[idClient-1],buffer);
        p_att->nbClientConnecte--;
        ressourceLoue->type = 2; // libération
        updateSysteme(p_att, ressourceLoue);
    unlockSysteme(sem_id);

    // envoie une notification si on a des ressources à libérer avant de quitter
    if(ressourceLoue->nbDemande>0){
        if(emmetreNotif(sem_idNotif) == -1){
            perror(BRED "Erreur : lors de l'envoie des notifications" reset);
        }
    }


    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror(BRED "Erreur : shmdt -> lors du détachement du segment mémoire." reset);
        exit(1);
    }
    exit(1);
}

void resetRequete(struct Requete_s* r){
    // reset de la requete
    for(int i=0;i<r->nbDemande;i++){
        r->tabDemande[i] = {0,0,0,0};
    }
    r->nbDemande = 0;
    r->type = 1;
}


