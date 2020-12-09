# HMIN105 - Projet d'un système de reservation distribué

## Instruction de Compilation

La commande suivante permet de créer l'exécutable client et l'exécutable server
```
make
```

La commande suivante permet de créer l'exécutable server
```
make server
```

La commande suivante permet de créer l'exécutable client
```
make client
```

La commande suivante permet de créer supprimer tous les fichiers mis à part les fichiers sources.
```
make clean
```

## Instruction de Compilation

Après avoir compiler le code, pour démarer le server execute la commande suivante :
```
./server PORT
```

Pour démarer le client execute la commande suivante :
```
./client nomClient IP PORT
```

## Modifier/ajouter/supprimer des sites

Pour modifier les sites présent sur le système, il suffit de moddifier le fichier : sites.txt, en respectant le format suivant :
```
nomDuSite : cpu stockage
```
<u>Remarque :</u> l'unité du stockage est "Go".
