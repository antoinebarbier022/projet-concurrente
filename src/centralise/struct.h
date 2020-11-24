#ifndef H_GL_STRUCT
#define H_GL_STRUCT

/* ------------------------------------------- */
/* Structure de données pour l'état du système */
/* ------------------------------------------- */

// information : on ajoute _s après le nom de la variable pour définir que c'est une structure utilisé pour l'état du système

struct SystemState_s{
    struct Site_s* sites;                 // Tableau qui contient tous les sites présent dans l'état du système
};

struct Site_s{
    unsigned int id;            // identifiant du site
    unsigned int cpu;           // Nombre de CPU total sur le site
    float sto;                  // Nombre de Stockage en Go sur le site

    struct Use_s *exclusiveMode;  // tableau qui contient toute les utilisations en mode exclusif
    struct Use_s *shareMode;      // tableau qui contient toute les utilisations en mode partagé
};

struct Use_s{
    unsigned int idCustomer;    // identifiant du client
    unsigned int cpu;           // Nombre de cpu utilisé par le client 
    float sto;                  // Nombre de Stockage en Go utilisé par le client
};




/* ------------------------------------------------------ */
/* Structure de données pour les modifications du système */
/* ------------------------------------------------------ */

struct Modification_m{
    int type;                   // 1 : demande | 2 : libération
    struct Use_m *exclusiveMode;
    struct Use_m *shareMode;
};

struct Use_m{
    unsigned int idSite;        // identifiant du site
    unsigned int cpu;           // Nombre de cpu utilisé par le client 
    float sto;                  // Nombre de Stockage en Go utilisé par le client
};



/* --------------------------------------------------- */
/* Structure de données pour les reponses aux requêtes */
/* --------------------------------------------------- */

struct Response{
    int type ; // 1 : confirmation | 2 : attente des ressources | 3 : erreur
};

#endif


