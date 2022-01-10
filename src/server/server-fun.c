#include "server-fun.h"

int server_list_task() {
   // TODO : Lire toutes les taches (parcourir le répertoire)
   // TODO : envoyer proprement les taches
   return 0;      
}

int server_create_task(int fd_req, char* rep_pipe, DIR* tasksdir, uint64_t first_id_available, int pid_run_fd, char* username) {
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
         }
         int fd_task;
         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/command", username, t_id);
         fd_task = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
         char* tstring = malloc(50);
         timing_string_from_timing(tstring, t);
         printf("%s\n", tstring);
         for (int i = 0; i < nbargs; i++) {
            write(fd_task, toexec[i], strlen(toexec[i]));
         }

         int task_stdout, task_stderr;
         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stdout", username, t_id);
         task_stdout = open(t_idname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stderr", username, t_id);
         task_stderr = open(t_idname, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);

         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/exitcodes", username, t_id);
         fd_task = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
         close(fd_task);

         dup2(task_stdout, STDOUT_FILENO);
         dup2(task_stderr, STDERR_FILENO);
         close(task_stdout);
         close(task_stderr);

         while(1) {
            sleep(10);
            // TODO : Attendre le temps donné dans la timing t
            // sleep(60*(t->minutes) + 3600*(t->hours) + 24*3600*(t->daysofweek));
            execvp(toexec[0], toexec);
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
   return 0;
}

int server_terminate(int fd_req, char* rep_pipe, char* pid_dir) {
   close(fd_req);
   uint16_t reptype = htobe16(SERVER_REPLY_OK);
   int fd_rep = open(rep_pipe, O_WRONLY);
   write(fd_rep, &reptype, sizeof(uint16_t));
   close(fd_rep);

   // kill les taches en cours
   char* buffer = malloc(MAX_BUFFER);
   int fd = open(pid_dir, O_RDONLY);
   while (1) {
      int nb_read = read(fd, buffer, MAX_BUFFER);
      if (nb_read == 0) {
         break;
      }
      char* token = strtok(buffer, ":");
      while (token != NULL) {
         token = strtok(NULL, "\n");
         kill(atoi(token), SIGKILL);
         token = strtok(NULL, ":");
      }
   }

   // supprime pidsrunning
   remove(pid_dir);

   return EXIT_SUCCESS;
}

int server_remove() {
   // TODO : lire l'id de la tâche à supprimer
   // TODO : kill le processus correspondant
   // TODO : supprimer le dossier de la tâche
   return 0;  
}

int server_times_exit() {
   // TODO : lire l'id de la tâche à lire
   // TODO : lire le fichier de sortie de la tâche
   return 0;     
}

int server_stdout() {
   // TODO : lire l'id de la tâche à lire
   // TODO : renvoie stdout
   return 0;
}

int server_stderr() {
   // TODO : lire l'id de la tâche à lire
   // TODO : renvoie stderr
   return 0;
}