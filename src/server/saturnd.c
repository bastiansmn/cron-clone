#include "saturnd.h"

int server_pid;

uint64_t first_id_available = 0;

void start_server() {
   // TODO : Ne pas relancer le serveur si il est déjà lancé
   // Chercher les taches existentes et les lancer

   int pid = fork();

   switch (pid) {
      case -1: {
         perror("fork");
         exit(EXIT_FAILURE);
      }
      case 0: {
         // Processus fils
         
         server_pid = getpid();
         // TODO cherche le plus petit id disponible pour first_id_available
         // TODO fermer les STDOUT/...
         break;
      }
      default: {
         // Processus père
         exit(0);
      }
   }
}

int main(int argc, char const *argv[]){
   
   start_server();

   int err;
   uint16_t op;

   char* username = getenv("USER");
   char req_pipe[50];
   char rep_pipe[50];
   sprintf(req_pipe, "/tmp/%s/saturnd/pipes/request", username);
   sprintf(rep_pipe, "/tmp/%s/saturnd/pipes/reply", username);
   
   char dirname[50];
   char tasksdirname[50];
   sprintf(dirname, "/tmp/%s", username);
   DIR* userdir = opendir(dirname);
   if (userdir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   closedir(userdir);
   sprintf(dirname, "/tmp/%s/saturnd", username);
   DIR* saturndir = opendir(dirname);
   if (saturndir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   closedir(saturndir);
   sprintf(dirname, "/tmp/%s/saturnd/pipes", username);
   DIR* pipesdir = opendir(dirname);
   if (pipesdir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   closedir(pipesdir);
   sprintf(tasksdirname, "/tmp/%s/saturnd/tasks", username);
   DIR* tasksdir = opendir(tasksdirname);
   if (tasksdir == NULL)
      mkdir(tasksdirname, S_IRUSR | S_IWUSR | S_IXUSR);

   char* pid_dir = malloc(50);
   sprintf(pid_dir, "/tmp/%s/saturnd/pidsrunning", username);
   int pid_run_fd = open(pid_dir, O_WRONLY | O_CREAT, S_IRWXU | S_IWUSR);

   int err_createfifo;
   err_createfifo = mkfifo(req_pipe, 0600);
   if (errno != EEXIST && err_createfifo == -1) {
      perror("req_pipe");
      exit (EXIT_FAILURE);
   }
   err_createfifo = mkfifo(rep_pipe, 0600);
   if (errno != EEXIST && err_createfifo == -1) {
      perror("rep_pipe");
      exit (EXIT_FAILURE);
   }

   int fd_req = -1; 
   int fd_rep = -1;

   while (1) {

      fd_req = open(req_pipe, O_RDONLY);
      if (fd_req == -1) {
         perror("open");
         goto error;
      }
      err = read(fd_req, &op, sizeof(uint16_t));
      if (err == -1) {
         perror("read");
         goto error;
      }
      
      switch (htobe16(op)) {
         case CLIENT_REQUEST_LIST_TASKS: {
            // TODO : Lire toutes les taches (parcourir le répertoire)
            // TODO : envoyer proprement les taches
            break;
         }
         case CLIENT_REQUEST_CREATE_TASK: {
            timing* t = malloc(sizeof (timing));
            read(fd_req, &(t->minutes), sizeof (uint64_t));
            read(fd_req, &(t->hours), sizeof (uint32_t));
            read(fd_req, &(t->daysofweek), sizeof (uint8_t));
            t -> minutes = htobe64(t->minutes);
            t -> hours = htobe32(t->hours);

            uint32_t nbargs;
            read(fd_req, &nbargs, sizeof (uint32_t));
            nbargs = htobe32(nbargs);

            char* toexec[nbargs];
            int command_size = 0;

            for (int i = 0; i < nbargs; i++) {
               uint32_t L;
               read(fd_req, &L, sizeof (uint32_t));
               L = htobe32(L); 
               command_size += (L+2);
               char* val = malloc(L+2);
               read(fd_req, val, L);
               val[L] = '\n';
               val[L+1] = '\0';
               toexec[i] = val;
            }
            close(fd_req);
            
            int task_pid = fork();
            switch (task_pid) {
               case -1: {
                  perror("fork");
                  exit(EXIT_FAILURE);
               }
               case 0: {
                  // Processus fils
                  int t_id = first_id_available;
                  char t_idname[50];
                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d", username, t_id);
                  int err_mkdir = mkdir(t_idname, S_IRUSR | S_IWUSR | S_IXUSR);
                  if (errno != EEXIST && err_mkdir == -1) {
                     perror("mkdir");
                     closedir(tasksdir);
                     goto error;
                  }
                  int fd_task;
                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/command", username, t_id);
                  fd_task = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
                  for (int i = 0; i < nbargs; i++) {
                     write(fd_task, toexec[i], strlen(toexec[i]));
                  }

                  int task_stdoud, task_stderr;
                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stdout", username, t_id);
                  task_stdoud = open(t_idname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stderr", username, t_id);
                  task_stderr = open(t_idname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);

                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/exitcodes", username, t_id);
                  fd_task = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
                  close(fd_task);

                  dup2(task_stdoud, STDOUT_FILENO);
                  dup2(task_stderr, STDERR_FILENO);

                  while(1) {
                     sleep(10);
                     // TODO : Attendre le temps donné dans la timing t
                     // sleep(60*(t->minutes) + 3600*(t->hours) + 24*3600*(t->daysofweek));
                     execvp(toexec[0], toexec);
                     printf("exec: %s\n", toexec[0]);
                     // TODO : écrire dans exitcodes la date du run et son exitcode
                  }

                  exit(EXIT_SUCCESS);
                  break;
               }
               default: {
                  // père continue
                  uint16_t reptype = htobe16(SERVER_REPLY_OK);
                  int fd_rep = open(rep_pipe, O_WRONLY);
                  write(fd_rep, &reptype, sizeof(uint16_t));
                  uint64_t tempid = htobe64(first_id_available);
                  write(fd_rep, &tempid, sizeof(uint64_t));
                  close(fd_rep);
                  
                  char* id_to_string = malloc(50);
                  sprintf(id_to_string, "%ld:%d\n", first_id_available, task_pid);
                  write(pid_run_fd, id_to_string, strlen(id_to_string));

                  first_id_available++;
                  break;
               }
            }

            
            break;
         }
         case CLIENT_REQUEST_REMOVE_TASK: {
            // TODO : lire l'id de la tâche à supprimer
            // TODO : kill le processus correspondant
            // TODO : supprimer le dossier de la tâche
            break;
         }
         case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
            // TODO : lire l'id de la tâche à lire
            // TODO : lire le fichier de sortie de la tâche
            break;
         }
         case CLIENT_REQUEST_TERMINATE: {
            close(fd_req);
            uint16_t reptype = htobe16(SERVER_REPLY_OK);
            fd_rep = open(rep_pipe, O_WRONLY);
            write(fd_rep, &reptype, sizeof(uint16_t));
            close(fd_rep);

            // kill les taches en cours
            FILE* file = fopen(pid_dir, "r"); 

            if(!file){
               printf("Unable to open : %s ", pid_dir);
               return -1;
            }

            char line[50];

            while (fgets(line, sizeof(line), file)) {
               const char * separators = ":";

               int pid = -1;
               int i = 0;
               char * strToken = strtok ( line, separators );
               while ( strToken != NULL ) {
                  if (i != 0) {
                     pid = atoi(strToken);
                     kill(pid, SIGKILL);
                     break;
                  }
                  i++;
                  strToken = strtok ( NULL, separators );
               }
            }
            fclose(file);

            // supprime pidsrunning
            remove(pid_dir);

            exit(EXIT_SUCCESS);
            break;
         }
         case CLIENT_REQUEST_GET_STDOUT: {
            // TODO : lire l'id de la tâche à lire
            // TODO : renvoie stdout
            break;
         }
         case CLIENT_REQUEST_GET_STDERR: {
            // TODO : lire l'id de la tâche à lire
            // TODO : renvoie stderr
            break;
         }
      }
   }
   //TODO :faire un timeout en cas d'erreur 
   return 0;

 error:
   if (errno != 0) perror("main");
   close(fd_rep);
   close(fd_req);
   return EXIT_FAILURE;
}
  