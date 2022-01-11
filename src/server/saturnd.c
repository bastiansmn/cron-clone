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
   closedir(tasksdir);

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

      
      // TODO vérifier que le pipe existe avant de l'ouvrir
      fd_req = open(req_pipe, O_RDONLY);
      struct stat sb;
      int l = stat(req_pipe, &sb);
      if (l == -1) {
         perror("stat");
         server_terminate(fd_req, rep_pipe);
      }

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
            server_list_task(fd_req, rep_pipe);
            break;
         }
         case CLIENT_REQUEST_CREATE_TASK: {
            int res = server_create_task(fd_req, rep_pipe, tasksdir, first_id_available);
            if (res == -1) {
               perror("server_create_task");
               goto error;
            }
            first_id_available++;
            break;
         }
         case CLIENT_REQUEST_REMOVE_TASK: {
            server_remove(fd_req, rep_pipe);
            break;
         }
         case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
            server_times_exit(fd_req, rep_pipe);
            break;
         }
         case CLIENT_REQUEST_TERMINATE: {
            exit(server_terminate(fd_req, rep_pipe));
            break;
         }
         case CLIENT_REQUEST_GET_STDOUT: {
            server_stdout(fd_req, rep_pipe);
            break;
         }
         case CLIENT_REQUEST_GET_STDERR: {
            server_stderr(fd_req, rep_pipe);
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
  