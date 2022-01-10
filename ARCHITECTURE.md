# Architecture du projet 

## Organisation du code source :

Dans `include` nous retrouvons tous les fichiers header nécessaires au client et au serveur. Le répertoire `src` contient les fichiers .c répartis dans deux sous-dossiers :
- Le dossier `client/` :
Ce dossier contient le code relatif à `cassini`, notamment un fichier `cassini.c` qui traite les arguments passés en ligne de commande et utilise les fonctions de `client-request.c` pour envoyer les informations précisées.
- Le dossier `server/` :
Ce dossier contient le code relatif à `saturnd`, en particulier `saturnd.c` qui s'occupe de démarrer le server sous forme d'un démon. Il créé ensuite l'arborescence nécessaire dans `/tmp/<USERNAME>/saturnd/...` (les pipes, les dossiers de tâches, ...) et lit le tube de requête et appelle les fonctions de `server-fun.h`

Le dossier `src/` contient aussi les fichiers .c qui sont communs au serveur et au client.

## Arborescence sur le disque :

Le server créé l'arborescence suivante :

```js
/tmp/<USERNAME>/saturnd
├─ tasks/
│  └─ <TASK_ID>/
│     ├─ command
│     ├─ exitcodes
│     ├─ stderr
│     └─ stdout
├─ pipes/
│  ├─ reply
│  └─ request
└─ pidsrunning
```

La création d'une tâche créé un dossier `<TASK_ID>` dans tasks/ ainsi que les fichiers correspondants. Le dossier `pipes/` contient les tubes utilisés par le server et par le client (sans préciser l'option `-p`).
Le fichier pidsrunning contient les taches au format `<ID>:<PID>` qui sont actuellement programmées à être exécutées.