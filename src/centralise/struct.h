#ifndef H_GL_STRUCT
#define H_GL_STRUCT

#define NBSITES 10 // nombre de site maximum
/*On se dit que le système peux accepté 20 clients donc si chaque client prends des ressources 
sur les 10 sites il y aurait 200 places dans le tableau d'utilisation.*/
#define NBMAXUSE 200 // 

/* ------------------------------------------- */
/* Structure de données pour l'état du système */
/* ------------------------------------------- */

// information : on ajoute _s après le nom de la variable pour définir que c'est une structure
typedef struct SystemState_s SystemState_s;
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
    Use_s exclusiveMode[NBMAXUSE];  // tableau qui contient toute les utilisations en mode exclusif
    Use_s shareMode[NBMAXUSE];      // tableau qui contient toute les utilisations en mode partagé
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
    unsigned int idSite;        // identifiant du site
    unsigned int cpu;           // Nombre de cpu utilisé par le client 
    float sto;                  // Nombre de Stockage en Go utilisé par le client
};

struct Modification_m{
    int type;                       // 1 : demande | 2 : libération

    int nbExclusiveMode;            // taille du tableau exclusiveMode
    int nbShareMode;                // taille du tableau shareMode
    struct UseMod_s exclusiveMode[NBMAXUSE];  
    struct UseMod_s shareMode[NBMAXUSE];
};




/* --------------------------------------------------- */
/* Structure de données pour les reponses aux requêtes */
/* --------------------------------------------------- */

struct Response_s{
    int type ; // 1 : confirmation | 2 : attente des ressources | 3 : erreur
};

#endif


