# SY5-projet-2021-2022

Le projet du cours de systèmes d'exploitation (L3), 2021-2022

Ce dépôt contient :

  - l'[énoncé](enonce.md) du projet, complété de détails sur le
    [protocole](protocole.md) de communication entre le démon et son
    client,

  - un [script de test](run-cassini-tests.sh) du client (`cassini`), et
    le jeu de [tests](tests) correspondant, 


## Lancement du projet :

Vous pouvez compiler le projet via la commande 
```
make
```

Si vous souhaitez uniquement compiler le client ou uniquement le server :
```
make client
```
ou
```
make server
```

Pour utiliser les test fournis, utilisez 
```
make test
```

Une fois le projet compilé, il faut lancer le server via 
```
./saturnd
```
Et lancer des requêtes via
```
./cassini ...
```
Usage :
```
./cassini [OPTIONS] -l -> list all tasks
      or: cassini [OPTIONS]    -> same
      or: cassini [OPTIONS] -q -> terminate the daemon
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]
          -> add a new task and print its TASKID
             format & semantics of the "timing" fields defined here:
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html
             default value for each field is "*"
      or: cassini [OPTIONS] -r TASKID -> remove a task
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task
      or: cassini [OPTIONS] -e TASKID -> get the standard error
      or: cassini -h -> display this message

   options:
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)
```

## Notes :

L'option -x provoque une Segfault, mais nous n'avons pas pu terminer à temps.
Toutes les autres options sont parfaitement fonctionnelles.
Cependant, nous n'avons pas encore fait en sorte que lors du redémarrage du démon, les tâches reprennent leur exécution.
Ce projet à par ailleurs été réalisé à 2 au lieu de 3 personnes conseillées.