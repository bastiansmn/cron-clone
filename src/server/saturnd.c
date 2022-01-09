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

                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stdout", username, t_id);
                  fd_task = open(t_idname, O_RDWR | O_CREAT, S_IRWXU | S_IWUSR);
                  dup2(fd_task, STDOUT_FILENO);
                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stderr", username, t_id);
                  fd_task = open(t_idname, O_RDWR | O_CREAT, S_IRWXU | S_IWUSR);
                  dup2(fd_task, STDERR_FILENO);

                  sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/exitcodes", username, t_id);
                  fd_task = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IWUSR);
                  close(fd_task);

                  // TODO : exec la tache
                     // sleep(60*htobe64(t->minutes) + 3600*htobe32(t->hours) + 24*3600*(t->daysofweek));
                     // execvp(toexec[0], toexec);
                  
                  while(1) {
                     printf("task %d running\n", t_id);
                     sleep(10);
                  }
                  printf("fin de tache\n");

                  exit(EXIT_SUCCESS);

                  break;
               }
               default: {
                  // père continue
                  uint16_t reptype = htobe16(SERVER_REPLY_OK);
                  int fd_rep = open(rep_pipe, O_WRONLY);
                  write(fd_rep, &reptype, sizeof(uint16_t));
                  write(fd_rep, &first_id_available, sizeof(uint64_t));
                  first_id_available++;
                  close(fd_rep);
                  break;
               }
            }

            
            break;
         }
         case CLIENT_REQUEST_REMOVE_TASK: {
            break;
         }
         case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
            break;
         }
         case CLIENT_REQUEST_TERMINATE: {
            close(fd_req);
            uint16_t reptype = htobe16(SERVER_REPLY_OK);
            fd_rep = open(rep_pipe, O_WRONLY);
            write(fd_rep, &reptype, sizeof(uint16_t));
            close(fd_rep);
            exit(EXIT_SUCCESS);
            break;
         }
         case CLIENT_REQUEST_GET_STDOUT: {
            break;
         }
         case CLIENT_REQUEST_GET_STDERR: {
            break;
         }
      }
   }

   return 0;

 error:
   if (errno != 0) perror("main");
   close(fd_rep);
   close(fd_req);
   return EXIT_FAILURE;
}
  