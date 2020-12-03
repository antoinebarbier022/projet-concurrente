#ifndef H_GL_STRUCT
#define H_GL_STRUCT


/* ------------------------------------------- */
/* Structure de données pour l'état du système */
/* ------------------------------------------- */

// information : on ajoute _s après le nom de la variable pour définir que c'est une structure
typedef struct SystemState_s SystemState_s;
typedef struct Site_s Site_s;
typedef struct Use_s Use_s;

struct Use_s{
    int     isUse;              
    int     idClient;
    int     cpu;               // Nombre de cpu utilisé par le client 
    int     sto;               // Nombre de Stockage en Go utilisé par le client
};

struct Site_s{
    int     id;                 // identifiant du site
    char    nom[30];
    int     cpu;                // Nombre de CPU total sur le site
    int     sto;                // Nombre de Stockage total sur le site

    int     nbMaxClientEx;               
    int     nbMaxClientSh;                      
    Use_s   *tabUseExclusif;
    Use_s   *tabUseShare;          
};


struct SystemState_s{
    int nbSites;                    // Taille du tableau sites
    Site_s *sites;          // Tableau qui contient tous les sites présent dans l'état du système
};


#endif