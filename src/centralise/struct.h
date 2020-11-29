#ifndef H_GL_STRUCT
#define H_GL_STRUCT

#define NBSITES 10 // nombre de site maximum
#define NBMAXCLIENT 20 // on fixe la taille maximum de client 

/* ------------------------------------------- */
/* Structure de données pour l'état du système */
/* ------------------------------------------- */

// information : on ajoute _s après le nom de la variable pour définir que c'est une structure
typedef struct SystemState_s SystemState_s;
typedef struct Modification_s Modification_s;
typedef struct RessourceLoue_s RessourceLoue_s;
typedef struct Site_s Site_s;
typedef struct Use_s Use_s;

struct Use_s{
    int isUse;              // boolean pour dire que le client loue des ressources ou non
    int mode;
    int cpu;               // Nombre de cpu utilisé par le client 
    float sto;                      // Nombre de Stockage en Go utilisé par le client
};

struct Site_s{
    int id;                // identifiant du site
    int cpu;               // Nombre de CPU total sur le site
    float sto; 
    int cpuFree;               // Nombre de CPU total sur le site
    float stoFree;                      // Nombre de Stockage en Go sur le site
    int cpuExclusif;
    int maxCpuPartage;          // le nombre max de cpu partager

    int nbUse;            // taille du tableau
    Use_s tabUse[NBMAXCLIENT];  // tableau qui contient toute les utilisations
    /*Sur chaque site on a deux tableaux, pour les locations en mode exclusif ou partagé
    Dans ses tableaux, chaque index est reservé pour un client spécifique en fonction de l'id du client
    Un client à un id entre 1 et 200*/
};

/* On déclare NBSITES sites dans l'état du système car on ne peut pas faire passer
 de tableau dynamique dans une structure présente dans un segment de mémoire */
struct SystemState_s{
    int nbSites;                    // Taille du tableau sites
    Site_s sites[NBSITES];          // Tableau qui contient tous les sites présent dans l'état du système
};




/* ------------------------------------------------------ */
/* Structure de données pour les modifications du système */
/* ------------------------------------------------------ */

struct UseMod_s{
    int idSite;        // identifiant du site 
    int mode;
    int cpu;           // Nombre de cpu utilisé par le client 
    float sto;                  // Nombre de Stockage en Go utilisé par le client
};

struct Modification_s{
    int type;                       // 1 : demande | 2 : libération
    int idClient;
    int nbDemande;            // taille du tableau exclusiveMode
    struct UseMod_s tabDemande[NBMAXCLIENT];  // ici on place juste à la suite les une des autres les UseMod_s, l'index ne correspond pas au site
};

/* -------------------------------------------------------------------------- */
/* Structure de données sauvegarder toutes les ressources loué par le client */
/* -------------------------------------------------------------------------- */

struct RessourceLoue_s{           
    int nbRessources;               
    struct UseMod_s tabLocation[NBSITES];  // toutes les ressources actuellement loué 
};


/* --------------------------------------------------- */
/* Structure de données pour les reponses aux requêtes */
/* --------------------------------------------------- */

struct Response_s{
    int type ; // 1 : confirmation | 2 : attente des ressources | 3 : erreur
};

#endif