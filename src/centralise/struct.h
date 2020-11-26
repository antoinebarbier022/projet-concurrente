#ifndef H_GL_STRUCT
#define H_GL_STRUCT

#define NBSITES 10 // nombre de site maximum
#define NBDEMANDEMAX 20 //nombre de demande maximum d'un utilisateur est = à 2*NBSITES car il y a deux mode exclusif ou partagé sur chaque site
/*On se dit que le système peux accepté 20 clients donc si chaque client prends des ressources 
sur les 10 sites il y aurait 200 places dans le tableau d'utilisation.*/
#define NBMAXCLIENT 200 // on fixe la taille maximum de client 

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
    unsigned int idCustomer;        // identifiant du client
    unsigned int cpu;               // Nombre de cpu utilisé par le client 
    float sto;                      // Nombre de Stockage en Go utilisé par le client
};

struct Site_s{
    unsigned int id;                // identifiant du site
    unsigned int cpu;               // Nombre de CPU total sur le site
    float sto;                      // Nombre de Stockage en Go sur le site

    int nbExclusiveMode;            // taille du tableau exclusiveMode
    int nbShareMode;                // taille du tableau shareMode
    Use_s exclusiveMode[NBMAXCLIENT];  // tableau qui contient toute les utilisations en mode exclusif
    Use_s shareMode[NBMAXCLIENT];      // tableau qui contient toute les utilisations en mode partagé
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
    unsigned int mode;
    unsigned int cpu;           // Nombre de cpu utilisé par le client 
    float sto;                  // Nombre de Stockage en Go utilisé par le client
};

struct Modification_s{
    int type;                       // 1 : demande | 2 : libération
    int nbDemande;            // taille du tableau exclusiveMode
    struct UseMod_s tabDemande[NBDEMANDEMAX];  // ici on place juste à la suite les une des autres les UseMod_s, l'index ne correspond pas au site
};

/* -------------------------------------------------------------------------- */
/* Structure de données sauvegarder toutes les ressources loué par le client */
/* -------------------------------------------------------------------------- */

struct RessourceLoue_s{
    int nbExclusiveMode;            
    int nbShareMode;               
    struct UseMod_s exclusiveMode[NBSITES];  
    struct UseMod_s shareMode[NBSITES];
};


/* --------------------------------------------------- */
/* Structure de données pour les reponses aux requêtes */
/* --------------------------------------------------- */

struct Response_s{
    int type ; // 1 : confirmation | 2 : attente des ressources | 3 : erreur
};

#endif


