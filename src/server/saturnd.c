#include "../include/saturnd.h"

void daemonize() {
   int pid = fork();

   switch pid {
      
   }
}

int main(int argc, char const *argv[]){
   
   // TODO : Démarrer le serveur
   // cf CM5 (21-10) + code fork_opt.c 
   // Il faut fork le pid actuel, puiss kill le parent, le proc sera adopté par 1
   
   daemonize();


   // TODO : Lire les opérations sur le tube de requete en boucle (cf cours 02-12) puis écrire vers le tube de requête selon le résultat voulu
   // On lit d'abord l'OPCODE, puis
   // switch (opcode)
   // TODO ...
   // + ajouter des fonctions

   return 0;
}
 