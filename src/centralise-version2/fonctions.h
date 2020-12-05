#include "struct.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

// Fonctions pour le server

int initSysteme(SystemState_s *s,const char* nomFichier);
void afficherSystemeInitial(SystemState_s *s);




// Fonctions pour le client

void afficherSysteme(SystemState_s *s);
void afficherRessourcesLoue(Requete_s *ressourceLoue);
void affichageTraceReservation

void updateSysteme(SystemState_s *s, Requete_s *r);
void updateRessourceLoueLocal(Requete_s *r, Requete_s *ressourceLoue);


void lockSysteme(int sem_id);
void unlockSysteme(int sem_id);

int attribuerIdentifiantClient(SystemState_s *s, const char* nom);
int attribuerNumSemNotification(SystemState_s *s, int id);

int saisieTypeAction(); // 1, 2 ou -1 (-1 = quitter)
void saisieDemandeRessource(Requete_s *r, Requete_s *ressourceLoue);
void saisieDemandeLiberation(Requete_s *r, Requete_s *ressourceLoue);

int demandeRessourceValide(SystemState_s *s, Requete_s *r);
int demandeLiberationValide(SystemState_s *s, Requete_s *r);
int demandeValide(SystemState_s *s, Requete_s *r);

// Initialisation des tableaux de sémaphores
int initTableauSemSites(int sem_id, SystemState_s *s);

// on initialise le sembuf
// retourne le nombre d'opérations
int initOp(struct sembuf *op, Requete_s *r);

int initOpNotif(struct sembuf *op, SystemState_s *s);
int emmetreNotif(int idTabSem);


void supprimerClient();
