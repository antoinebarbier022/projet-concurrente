# HMIN105M - Projet

## Système de réservation distribué



L'objectif de ce document est de définir l'architecture de notre application. Nous allons voir comment nous avont définie dans notre projet : 

- Notre politique de réservation

- La structure état système
- Les objets IPC ainsi que le principe de leurs utilisations
- Les processus et threads ainsi que leurs roles
- Le protocole d’échange à distance utilisé



### Notre politique de réservation

Pour répondre aux besoins de ce projet, nous avons définie une politique de reservation. 

Le nombre de site maximum dans l'état du système est limité à 100.
Nous avons décider de définir qu'un cpu peut être partagé par 4 clients. De même, un espace de stockage de 1Go peut être partagé par 4 clients.
Ainsi sur un site contenant 50 cpu exclusifs, le client aura la possibilité de louer au maximum 200 cpu en mode partagé (car 4x50 = 200). 

Pour faire une réservation, un client doit réserver au minimum 1 cpu sur le site qu'il demande et à la possibilité de réserver ou non du stockage.

Ainsi, il ne peut y avoir plus de clients que de cpu en mode exclusif sur un site donné.

Quand un client réserve des cpu partagés, il réserve des "unités de cpu". Donc quand un utilisateur reserve plusieurs cpu partagé, les unités de cpu demandé peuvent être prise sur un cpu. 

Une fois qu'un utilisateur quitte la plate-forme, les ressources qu'il à loué sont libéré.



### La structure de l'état du système

Dans la structure de l'état du système, voici les informations dont nous auront besoin de sauvegarder :

- le nombre de sites dans le système
- les identifiants utilisateurs associé à leur nom
- les identifiants utilisateurs associé au numéro de leur sémaphore de notification
- les informations sur chaque sites :
  - l'identifiant du site
  - le nom du site
  - le nombre de cpu initiale sur le site
  - le nombre de stockage initial sur le site
  - les quantités de cpu et de stockages libre (en mode exclusif et en mode partagé) sur le site 
  - Les réservations en mode exclusif avec : 
    - l'identifiant du client
    - le nombre de cpu reservé
    - le nombre de stockage reservé
  - Les réservations en mode partagé avec : 
    - l'identifiant du client
    - le nombre de cpu reservé
    - le nombre de stockage reservé

 

### Les objets IPC et leurs principes d'utilisations

Voici les objets IPC que nous utilisont dans ce projet :

- **Un segment mémoire** : ce segment mémoire permet de partagé l'état du système entre tous les processus fils du server.

- **Deux tableaux de sémaphores** :

  - Le **tableau de sémaphores 1 (Les ressources)** est utilisé pour stocker le sémaphore qui permet de protéger le segment mémoire et de stocker tous les sémaphores qui représente les différentes ressources de chaque site.
    Le le sémaphore 0 est celui qui va permettre de protégé le segment mémoire. Ce sémaphore est initialisé à 1.
    Les sémaphores suivant sont initialisé avec des valeurs qui permette de représentant les ressources (cpu et stockage) dans les deux modes de réservation *[Voir le shéma ci-dessous]*.

    ![image-20201206162553875](./assets/image-20201206162553875.png)

  - Le **tableau de sémaphores 2 (Les notification)** est utilisé pour stocker un sémaphore propre à chaque client qui permet de représenter la présence ou l'absence d'une notification.
    Le sémaphore numéro 0 est la notification pour le processus serveur parent, afin d'avoir un affichage sur ce dernier.
    Quand la valeur d'un sémaphore du tableau est égale à un, cela signifie que le processus associé à ce sémaphore à reçu une notification (donc cela signifie que l'état du système à été modifié).



### Les processus et threads

Le système de réservation distribué comprend deux programmes : Un programme qui s’execute sur le server et un programme qui s’execute sur le client. Plusieurs clients différents peuvent se connecter sur le server. Dans la consigne il est noté que le processus server attent la connection des clients puis delègue la suite du travail à ses processus fils. Cela signifie que l'on doit faire une fork dans le programme du server afin que chaque client soit connecté avec un processus fils du server.

Le client utilise un thread qui permet de receptionner les messages du server et d'afficher la mise à jour de l'état du système.
Le programme principale du client se charge de récupérer les saisie de l'utilisateur et d'envoyer la requête au server.

Le server utilise deux threads : 
- Un thread pour vérifier que les objets ipc n'ont pas été tuer par une entité extérieur.
- un thread pour envoyé la notification avec le nouvelle état du server au client.

Le programme du server (ici le processus fils) ce charge de récupèrer les requètes des clients et d'executer l'action de la requête reçu.

  

### Le protocole d’échange à distance utilisé

Le protocole d'échange utilisé dans notre architecture est le protocole TCP, comme demandé dans la consigne du projet.