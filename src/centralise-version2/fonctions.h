#include "struct.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>


/**
 * \fn    initSysteme(SystemState_s *s,const char* nomFichier)
 * \brief Fonction qui récupère les données des sites depuis un fichier .txt et initialise une structure SystemState_s
 * \param [in,out] s : état du système que l'on va initialiser dans cette fonction.
 * \param [in] nomFichier : nom du fichier où sont stocker les sites.
 * \return -1 si erreur, 1 sinon
 */
int initSysteme(SystemState_s *s,const char* nomFichier);

/**
 * \fn    afficherSysteme(SystemState_s *s)
 * \brief Fonction qui affiche l'état du système sous forme de tableau
 * \param [in] s : On passe la structure de l'état du système
 */
void afficherSysteme(SystemState_s *s);

/**
 * \fn    afficherRessourcesLoue(Requete_s *ressourceLoue)
 * \brief Fonction qui affiche les ressources loué par le client
 * \param [in] ressourceLoue : On passe la structure de l'état du système
 */
void afficherRessourcesLoue(Requete_s *ressourceLoue);

/**
 * \fn    updateSysteme(SystemState_s *s, Requete_s *r)
 * \brief Fonction qui met à jour l'état du système avec la requête du client
 * \param [in,out] s : état du système que l'on va mettre à jour
 * \param [in] r : la requète du client
 */
void updateSysteme(SystemState_s *s, Requete_s *r);

/**
 * \fn    updateRessourceLoueLocal(Requete_s *r, Requete_s *ressourceLoue)
 * \brief Fonction qui met à jour la structure qui contient toutes les ressources loué par le client avec la nouvelle requête 
 * \param [in,out] r : Structure qui contient toutes les ressources loué par le client
 * \param [in] ressourceLoue : la requète du client
 */
void updateRessourceLoueLocal(Requete_s *r, Requete_s *ressourceLoue);

/**
 * \fn    lockSysteme(int sem_id)
 * \brief Fonction qui permet de protéger l'accès au segment mémoire (opération P)
 *        Cette fonction est atomique, et fait une opération P sur le sémaphore 0 du tableau de sémaphore identifier par le paramètre de la fonction 
 * \param [in] sem_id : identifiant du tableau de sémaphore
 */
void lockSysteme(int sem_id);

/**
 * \fn    void unlockSysteme(int sem_id)
 * \brief Fonction qui permet de protéger l'accès au segment mémoire (opération V)
 *        Cette fonction est atomique, et fait une opération V sur le sémaphore 0 du tableau de sémaphore identifier par le paramètre de la fonction 
 * \param [in] sem_id : identifiant du tableau de sémaphore
 */
void unlockSysteme(int sem_id);

/**
 * \fn    attribuerIdentifiantClient(SystemState_s *s, const char* nom)
 * \brief Fonction qui permet d'attribuer un identifiant au client
 *        Dans l'état du système, il y a un tableau de strings qui permet d'associer le nom d'utilisateur avec un identifiant Client.
 *        Dans cette fonction, on recherche dans le tableau la première zone mémoire vide et on y place le nom de l'utilisateur.
 * \param [in,out] s : état du système afin d'accèder au tableau
 * \param [in] nom : le nom du client pour le placer dans le tableau
 * \return le numéro de l'identification du client (l'identifiant commence à 1)
 */
int attribuerIdentifiantClient(SystemState_s *s, const char* nom);

/**
 * \fn    attribuerNumSemNotification(SystemState_s *s, int id)
 * \brief Fonction qui permet d'attribuer un numéro de sémaphore de notification au client
 *        Dans l'état du système, il y a un tableau de d'entier qui permet d'associer l'id de l'utilisateur avec un numéro de sémaphore.
 *        Dans cette fonction, on recherche dans le tableau la première zone mémoire vide et on y place l'id de l'utilisateur.
 *        L'index du tableau représente le numéro de sémaphore
 * \param [in,out] s : état du système afin d'accèder au tableau
 * \param [in] id : l'identifiant du client pour le placer dans le tableau
 * \return le numéro du semaphore de notification attribué au client (le numéro de sémaphore commence à 1 car le sémaphore 0 est reservé pour le server)
 */
int attribuerNumSemNotification(SystemState_s *s, int id);


/**
 * \fn    saisieTypeAction()
 * \brief Fonction qui permet de demandé à l'utilisateur l'action qu'il souhaite faire
 *          - Demander des ressources : la fonction retournera 1
 *          - Libérer des ressources  : la fonction retournera 2
 *          - Quitter le programme    : la fonction retournera 3
 *          Si il y a une erreur dans la saisie, on affiche le message d'erreur et on retourne 3 pour quitter
 *          
 * \return le type de l'action, sinon 3 pour quitter
 */
int saisieTypeAction();

/**
 * \fn    saisieDemandeRessource(Requete_s *r, Requete_s *ressourceLoue)
 * \brief Demande à l'utilisateur de saisir les ressources qu'il souhaite réservé
 *        Cette fonction demande une reservation et l'ajoute dans la requête du client
 *        On vérifie aussi que la demande saisie n'est pas déja reservé avec la structure qui contient toutes les ressources loué par l'utilisateur
 * \param [out] r : On modifie la requête de l'utilisateur pour ajouter sa nouvelle demande
 * \param [in] ressourceLoue : Contient toutes les réservations de l'utilisateur
 * \return 1 ,sinon si il y a une erreur : -1
 */
int saisieDemandeRessource(Requete_s *r, Requete_s *ressourceLoue);

/**
 * \fn    saisieDemandeLiberation(Requete_s *r, Requete_s *ressourceLoue)
 * \brief Demande à l'utilisateur de saisir l'identifiant du site qu'il souhaite libérer
 *        Cette fonction demande le nom du site que l'on veut libérer et on l'ajoute dans la requête du client
 *        On vérifie aussi que le site en question est en cours de réservation, sinon on renvoie une erreur
 * \param [out] r : On modifie la requête de l'utilisateur pour ajouter sa nouvelle demande de libération
 * \param [in] ressourceLoue : Contient toutes les réservations de l'utilisateur
 * \return 1 ,sinon si il y a une erreur : -1
 */
int saisieDemandeLiberation(Requete_s *r, Requete_s *ressourceLoue);

/**
 * \fn    demandeRessourceValide(SystemState_s *s, Requete_s *r)
 * \brief Cette fonction test si la requête de demande de ressource est valide
 *        On verifie que la demande de l'utilisateur est correct, c'est à dire qu'il ne puisse pas demander 
 *        un site qui n'existe pas, ou bien demander trop de cpu et stockage par rapport au cpu et stockage initial sur le site
 * \param [in] s : Etat du système
 * \param [in] r : Requête de l'utilisateur
 * \return 1 si la requête est valide, -1 sinon
 */
int demandeRessourceValide(SystemState_s *s, Requete_s *r);

// Commentaire sur la fonction suivante : (elle ne sert pas vraiment car normalement on fait une vérification au moment de la saisie)
/**
 * \fn    demandeLiberationValide(SystemState_s *s, Requete_s *r)
 * \brief Cette fonction test si la requête de libération de ressource est valide
 *        On verifie que la demande de l'utilisateur est correct, c'est à dire qu'il ne puisse pas demander de libérer
 *        les ressources d'un site qui n'existe pas.
 * \param [in] s : Etat du système
 * \param [in] r : Requête de l'utilisateur
 * \return 1 si la requête est valide, -1 sinon
 */
int demandeLiberationValide(SystemState_s *s, Requete_s *r);

/**
 * \fn    demandeValide(SystemState_s *s, Requete_s *r)
 * \brief Cette fonction test si la requête est valide
 *        Dans cette fonction on fait simplement appel au fonctions : 
 *          - demandeRessourceValide
 *          - demandeLiberationValide
 * \param [in] s : Etat du système
 * \param [in] r : Requête de l'utilisateur
 * \return 1 si la requête est valide, -1 sinon
 */
int demandeValide(SystemState_s *s, Requete_s *r);


/**
 * \fn    initTableauSemSites(int sem_id, SystemState_s *s)
 * \brief Initialisation du tableaux de sémaphores qui contient les ressources de tous les sites
 *        Cette fonction initialise chaque sémaphore de façon différente :
 *          - pour un sémaphore cpu exclusif d'un site : (cpuSite multiplier par 4)
 *          - pour un sémaphore cpu partagé d'un site  : (cpuSite multiplier par 4) multiplier par CPUSHARED
 *          - pour un sémaphore sto exclusif d'un site : (stoSite multiplier par 4)
 *          - pour un sémaphore sto partagé d'un site  : (stoSite multiplier par 4) multiplier par CPUSHARED
 * 
 *        Remarque : CPUSHARED est équivalant au nombre de client qui peuvent ce partagé 1 cpu
 * \param [in] sem_id : identifiant du sémaphore
 * \param [in] s : Etat du système
 * \return 1 si tout va bien, -1 sinon
 */
int initTableauSemSites(int sem_id, SystemState_s *s);

/**
 * \fn    initOp(struct sembuf *op, Requete_s *r)
 * \brief Cette fonction initialise la structure sembuf avec toutes les opérations que l'on souhaite faire sur les sémaphores
 *        Il faut aller voir le code et les commentaires sur le code pour comprendre les calcules fait pour identifier 
 *        les sémaphores et choisir les opérations à faire sur chaque sémaphore.
 *      
 * \param [in, out] op : sembuf que l'on initialise
 * \param [in] r : Requête du client
 * \return retourne le nombre d'opération dans le sembuf
 */
int initOp(struct sembuf *op, Requete_s *r);

/**
 * \fn    emmetreNotif(int idTabSem)
 * \brief Emmetre une notification
 *        Pour emmettre une notification, cette fonction prend en paramètre l'identifiant du tableau de sémaphores 
 *        des notification et fait un SETVAL de 1 sur tous les sémaphores
 * \param [in] idTabSem : identifiant du tableau de sémaphore
 * \return -1 si erreur, 1 sinon
 */
int emmetreNotif(int idTabSem);

/**
 * \fn    fermerClient
 * \brief Cette fonction se charge de supprimer le client de l'état du système
 *        Remarque : les sémaphores rendent automatiquement les ressources reservé par le client (grâce au SEM_UNDO)
 */
void fermerClient(int idClient, int numSemNotif, struct SystemState_s* p_att, Requete_s *ressourceLoue, int shm_id, int sem_id, int sem_idNotif);
