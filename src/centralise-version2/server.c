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
        nbSite++;
    }
    rewind(fichier); // on replace le curseur au début du fichier

    s.nbSites = nbSite;
    s.sites = (Site_s*)malloc(nbSite*sizeof(Site_s));

    while(!feof(fichier)){
        fscanf(fichier, "%s : %d %d\n",name, &cpu, &sto); // on a deja regarder si il y avait une erreur donc pas de vérification à faire
        id++;

        s.sites[id-1].id = id;
        strcpy(s.sites[id-1].nom, name);
        s.sites[id-1].cpu = cpu;
        s.sites[id-1].sto = sto;
        s.sites[id-1].nbMaxClientEx = cpu;
        s.sites[id-1].nbMaxClientSh = 4*cpu; // 4 = le nombre de partage possible d'un cpu
        s.sites[id-1].tabUseExclusif = (Use_s*)malloc(s.sites[id-1].nbMaxClientEx*sizeof(Use_s));
        s.sites[id-1].tabUseShare = (Use_s*)malloc(s.sites[id-1].nbMaxClientSh*sizeof(Use_s));
    }    
    fclose(fichier);

    return s;
}

void afficherInitEtatSysteme(SystemState_s *s){
    printf("\033[36m "); // couleur du texte
    printf("\nEtat du système : (%d sites)\n", s->nbSites);
    for(int i=0;i<s->nbSites;i++){
        printf("  - %-15s[id=%d] : %-4d cpu, %-4d Go, nbResExclusif : %d, nbResShare : %d \n",   
                s->sites[i].nom, 
                s->sites[i].id, 
                s->sites[i].cpu ,
                s->sites[i].sto ,
                s->sites[i].nbMaxClientEx,
                s->sites[i].nbMaxClientSh);
    }
    printf("\033[m");
}

int main(int argc, char const *argv[])
{
    SystemState_s etatSysteme;
    etatSysteme = initEtatSysteme("sites.txt");
    afficherInitEtatSysteme(&etatSysteme);
    
    return 0;
}
