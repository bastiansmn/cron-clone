#include "../include/saturnd.h"

int server_pid;

void start_server() {
   int pid = fork();

   switch (pid) {
      case -1: {
         perror("fork");
         exit(EXIT_FAILURE);
      }
      case 0: {
         // Processus fils
         server_pid = getpid();
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
   sprintf(dirname, "/tmp/%s", username);
   DIR* userdir = opendir(dirname);
   if (userdir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   closedir(userdir);
   sprintf(dirname, "/tmp/%s/saturnd", username);
   DIR* saturndir = opendir(dirname);
   if (saturndir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   sprintf(dirname, "/tmp/%s/saturnd/pipes", username);
   DIR* pipesdir = opendir(dirname);
   if (pipesdir == NULL)
      mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
   closedir(pipesdir);

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
   // https://linux.die.net/man/2/lstat
   // http://manpagesfr.free.fr/man/man3/mkfifo.3.html

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
  