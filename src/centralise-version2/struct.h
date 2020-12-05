#ifndef H_GL_STRUCT
#define H_GL_STRUCT

// 
#define NBCPUMAX_EXCLUSIF 500
#define CPUSHARED 4 // un cpu peut etre partagé par 4
#define NBSITEMAX 100
#define NBSIZEMAXNAME 30

#define NBCPUMAX_SHARE CPUSHARED*NBCPUMAX_EXCLUSIF
#define NBCLIENTMAX NBCPUMAX_SHARE
#define NBOPMAXRESSOURCE 4*NBSITEMAX // Car un site à 4 semaphore

/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

//Regular text
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define BWHT "\e[1;37m"

//Regular underline text
#define UBLK "\e[4;30m"
#define URED "\e[4;31m"
#define UGRN "\e[4;32m"
#define UYEL "\e[4;33m"
#define UBLU "\e[4;34m"
#define UMAG "\e[4;35m"
#define UCYN "\e[4;36m"
#define UWHT "\e[4;37m"

//Regular background
#define BLKB "\e[40m"
#define REDB "\e[41m"
#define GRNB "\e[42m"
#define YELB "\e[43m"
#define BLUB "\e[44m"
#define MAGB "\e[45m"
#define CYNB "\e[46m"
#define WHTB "\e[47m"

//High intensty background 
#define BLKHB "\e[0;100m"
#define REDHB "\e[0;101m"
#define GRNHB "\e[0;102m"
#define YELHB "\e[0;103m"
#define BLUHB "\e[0;104m"
#define MAGHB "\e[0;105m"
#define CYNHB "\e[0;106m"
#define WHTHB "\e[0;107m"

//High intensty text
#define HBLK "\e[0;90m"
#define HRED "\e[0;91m"
#define HGRN "\e[0;92m"
#define HYEL "\e[0;93m"
#define HBLU "\e[0;94m"
#define HMAG "\e[0;95m"
#define HCYN "\e[0;96m"
#define HWHT "\e[0;97m"

//Bold high intensity text
#define BHBLK "\e[1;90m"
#define BHRED "\e[1;91m"
#define BHGRN "\e[1;92m"
#define BHYEL "\e[1;93m"
#define BHBLU "\e[1;94m"
#define BHMAG "\e[1;95m"
#define BHCYN "\e[1;96m"
#define BHWHT "\e[1;97m"

//Reset
#define reset "\e[0m"

/* ------------------------------------------- */
/* Structure de données pour l'état du système */
/* ------------------------------------------- */

// information : on ajoute _s après le nom de la variable pour définir que c'est une structure
typedef struct SystemState_s SystemState_s;
typedef struct Site_s Site_s;
typedef struct Use_s Use_s;
typedef struct Requete_s Requete_s;
typedef struct Demande_s Demande_s;


struct Use_s{
    int     isUse;              
    int     idClient;
    int     cpu;               // Nombre de cpu utilisé par le client 
    int     sto;               // Nombre de Stockage en Go utilisé par le client
};

struct Site_s{
    int     id;                 // identifiant du site
    char    nom[NBSIZEMAXNAME];
    int     cpu;                // Nombre de CPU total sur le site
    int     sto;                // Nombre de Stockage total sur le site

    float     resCpuExFree;
    float     resCpuShFree;
    float     resStoExFree;
    float     resStoShFree;

    int     nbMaxClientEx;               
    int     nbMaxClientSh;                      
    Use_s   tabUseExclusif[NBCPUMAX_EXCLUSIF];
    Use_s   tabUseShare[NBCPUMAX_SHARE];          
};

    // mettre dans le systeme :
    // - un tableau qui associe le nom avec un identifiant (index du tableau)
    // - un tableau qui associe le nom avec un numero de semaphore de notif
    // quand le client quitte on l'enlève du tableau

struct SystemState_s{
    char    tabId[NBCLIENTMAX][NBSIZEMAXNAME];    //tableau de string (les noms des clients et un idex qui est leur id)
    int     tabNotif[NBCLIENTMAX];
    int     nbClientConnecte;
    int     nbSites;                    // Taille du tableau sites
    Site_s  sites[NBSITEMAX];          // Tableau qui contient tous les sites présent dans l'état du système
};


/* ------------------------------------ */
/* Structure de données pour la requête */
/* ------------------------------------ */

struct Demande_s{
    int     idSite;
    int     mode;
    int     cpu;
    int     sto;
};

struct Requete_s{
    int             type;
    int             idClient;
    int             nbDemande;
    Demande_s       tabDemande[NBSITEMAX];
};




#endif