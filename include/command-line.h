#ifndef SY5_PROJET_2021_2022_COMMAND_LINE_H
#define SY5_PROJET_2021_2022_COMMAND_LINE_H

#include <stdint.h>
#include <stringc.h>

typedef struct commandline {
    uint32_t argc;
    stringc* argv;// on peut aussi utiliser "string* argv" c'est pareil
    /*pas besoin de decouper car il existe deja optarg donc on peut utiliser directement
     * argv+optarg et argc-optarg pour avoir la commande a envoyer(regarde exemple telephone) */
} commandline ;

#endif
