#include "struct.h"

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

using namespace std;

SystemState_s initEtatSysteme(const char* nomFichier){

    FILE* fichier = NULL;
    fichier = fopen(nomFichier,"r"); // ouverture du fichier en lecture seul
    if(fichier == NULL){
        perror("Erreur : Impossible d'ouvrir le fichier d'initialisation des sites.");
        exit(1);
    }

    SystemState_s s; // le système que l'on va retourner
    
    char name[30];
    int id, cpu, sto;
    id = 0;
    int nbSite = 0;
    while(!feof(fichier)){
        if(fscanf(fichier, "%s : %d %d\n",name, &cpu, &sto) != 3){
            perror("Erreur : Le fichier d'initialisation des sites est mal formé.");
        exit(1);
        }
        if(cpu > 500){
            perror("Erreur : Le nombre de cpu sur un site ne peut pas être supérieur à 500 : (vous devez modifier le fichier site.txt)");
            exit(1);
        }
        nbSite++;
    }
    rewind(fichier); // on replace le curseur au début du fichier

    s.nbSites = nbSite;

    while(!feof(fichier)){
        fscanf(fichier, "%s : %d %d\n",name, &cpu, &sto); // on a deja regarder si il y avait une erreur donc pas de vérification à faire
        id++;

        s.sites[id-1].id = id;
        strcpy(s.sites[id-1].nom, name);
        s.sites[id-1].cpu = cpu;
        s.sites[id-1].sto = sto;
        s.sites[id-1].nbMaxClientEx = cpu;
        s.sites[id-1].nbMaxClientSh = 4*cpu; // 4 = le nombre de partage possible d'un cpu
    }    
    fclose(fichier);

    return s;
}

void afficherInitEtatSysteme(SystemState_s *s){
    printf("\033[36m "); // couleur du texte
    printf("\nEtat du système : (%d sites)\n", s->nbSites);
    for(int i=0;i<s->nbSites;i++){
        printf("  - %-15s[id=%d] : %-4d cpu, %-4d Go, nbResExclusif : %-4d, nbResShare : %-4d \n",   
                s->sites[i].nom, 
                s->sites[i].id, 
                s->sites[i].cpu ,
                s->sites[i].sto ,
                s->sites[i].nbMaxClientEx,
                s->sites[i].nbMaxClientSh);
    }
    printf("\033[m\n");
}


int main(int argc, char const *argv[])
{
    SystemState_s etatSysteme;
    etatSysteme = initEtatSysteme("sites.txt");
    afficherInitEtatSysteme(&etatSysteme);


    // récuperer les identifiants des ipc
    key_t cle = ftok(argv[1], 'e');

    // identification du segment mémoire pour l'état su système
    int shm_id = shmget(cle, sizeof(SystemState_s), IPC_CREAT|0666);
    if(shm_id == -1) {
        perror("erreur : shmget -> identification segment mémoire pour l'état du système");
        exit(1);
    }

    //attachement au segement memoire
    struct SystemState_s* p_att = (struct SystemState_s*)shmat(shm_id,NULL,0); 
    if((struct SystemState_s*)p_att == (void*)-1) {
        perror("erreur : shmat -> lors de l'attachement au segment mémoire");
        exit(1);
    }

    // on initialise le segment de mémoire
    *p_att = etatSysteme;

    // détachement du segment mémoire
    int dtres = shmdt(p_att); 
    if(dtres == -1){
        perror("Erreur : shmdt -> lors du détachement du segment mémoire.");
        exit(1);
    }

    int touche;
    cout << "Appuyer sur une touche pour arreter le server ..." << endl;
    cin >> touche;


    // Destruction du segment mémoire
    shmctl(shm_id,0,IPC_RMID);
    
    return 0;
}
