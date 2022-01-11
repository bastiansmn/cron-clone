#include "server-fun.h"

int server_list_task(int fd_req, char* rep_pipe) {
   close(fd_req);
   
   char taskdir[50];
   char* username = getenv("USER");
   sprintf(taskdir, "/tmp/%s/saturnd/tasks", username);
   char name[50]; 
   DIR* dirp = opendir(taskdir);
   struct dirent* entry;
   uint32_t c = 0;
   while ((entry = readdir(dirp)))
      if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0))
         c++;
   closedir(dirp);

   int fd_rep = open(rep_pipe, O_WRONLY);
   uint16_t reptype = htobe16(SERVER_REPLY_OK);
   write(fd_rep, &reptype, sizeof(uint16_t));
   uint32_t nb_tasks = htobe32(c);
   write(fd_rep, &nb_tasks, sizeof(uint32_t));
   
   dirp = opendir(taskdir);
   while ((entry = readdir(dirp))) {
      if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
         uint64_t taskid = strtoull(entry->d_name, NULL, 10);
         uint64_t temp_taskid = htobe64(taskid);
         write(fd_rep, &temp_taskid, sizeof(uint64_t));
         
         sprintf(name, "/tmp/%s/saturnd/tasks/%lu/command", username, taskid);
         int fd = open(name, O_RDONLY);
         int err_read;
         uint32_t argc = 0;
         char* buffer = malloc(MAX_BUFFER);
         // Compute argc and send timing struct to the client
         while (1) {
            err_read = read(fd, buffer, MAX_BUFFER);
            if (err_read > 0) {
               int i = 0;
               char * token = strtok(buffer, "\n");
               while (token != NULL) {
                  if (i == 0) { // If we need to parse the timing
                     char* minutes;
                     char* hours;
                     char* daysofweek;
                     int j = 0;
                     char* token2;
                     char* rest = token;
                     while ((token2 = strtok_r(rest, " \n", &rest))) {
                        j++;
                        if (j == 1)
                           minutes = token2;
                        else if (j == 2)
                           hours = token2;
                        else if (j == 3)
                           daysofweek = token2;
                        else
                           break;
                     }
                     free(token2);

                     timing* t = malloc(sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
                     timing_from_strings(t, minutes, hours, daysofweek);
                     t->minutes = htobe64(t->minutes);
                     t->hours = htobe32(t->hours);

                     write(fd_rep, t, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
                     free(t);
                  } else { // Count argc
                     argc++;
                  }
                  i++;
                  token = strtok ( NULL, "\n" );
               }
               free(token);
            } else {
               close(fd);
               break;
            }
         }
         free(buffer);
         
         argc = htobe32(argc);
         write(fd_rep, &argc, sizeof(uint32_t));
         fd = open(name, O_RDONLY);
         int i = 0;
         // Send all the args of the command
         buffer = malloc(MAX_BUFFER);
         while(1) {
            err_read = read(fd, buffer, MAX_BUFFER);
            if (err_read > 0) {
               char * token = strtok (buffer, "\n");
               while (token != NULL) {
                  if (i != 0) { // Command
                     uint32_t len = strlen(token);
                     uint32_t templen = htobe32(len);
                     write(fd_rep, &templen, sizeof(uint32_t));
                     write(fd_rep, token, len);
                  }
                  i++;
                  token = strtok ( NULL, "\n" );
               }
            } else {
               close(fd);
               break;
            }
         }
      }
   }
   close(fd_rep);
   closedir(dirp);
   // TODO : envoyer proprement les taches
   return 0;      
}

int server_create_task(int fd_req, char* rep_pipe, DIR* tasksdir, uint64_t first_id_available) {
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

   char* username = getenv("USER");
   
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
         fd_task = open(t_idname, O_WRONLY | O_CREAT, S_IRWXU | S_IWUSR);
         char tstring[50];
         int pos = timing_string_from_timing(tstring, t);
         tstring[pos] = '\n';
         tstring[pos+1] = '\0';
         write(fd_task, tstring, strlen(tstring));
         for (int i = 0; i < nbargs; i++) {
            write(fd_task, toexec[i], strlen(toexec[i]));
         }
         close(fd_task);

         int task_stdout, task_stderr;
         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stdout", username, t_id);
         task_stdout = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/stderr", username, t_id);
         task_stderr = open(t_idname, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/exitcodes", username, t_id);
         fd_task = open(t_idname, O_WRONLY | O_CREAT, S_IRWXU);

         sprintf(t_idname, "/tmp/%s/saturnd/tasks/%d/pid", username, t_id);
         int fd_pid = open(t_idname, O_WRONLY | O_CREAT, S_IRWXU);
         char* temp = malloc(20);
         sprintf(temp, "%d\n", getpid());
         write(fd_pid, temp, strlen(temp));
         close(fd_pid);


         char* minutesstr = malloc(50);
         timing_string_from_field(minutesstr, 0, 59, t->minutes);
         uint64_t minutes = strtoull(minutesstr, NULL, 10);

         char* hoursstr = malloc(50);
         timing_string_from_field(hoursstr, 0, 23, t->hours);
         uint32_t hours = strtoul(hoursstr, NULL, 10);

         char* daysofweekstr = malloc(50);
         timing_string_from_field(daysofweekstr, 0, 6, t->daysofweek);
         uint8_t daysofweek = strtoul(daysofweekstr, NULL, 10);

         
         dup2(task_stdout, STDOUT_FILENO);
         dup2(task_stderr, STDERR_FILENO);

         int err = 0;
         while(err != -1) {
            if (minutes == 0 && hours == 0 && daysofweek == 0) {
               sleep(10);
            }
            sleep(60*(minutes) + 3600*(hours) + 24*3600*(daysofweek));
            // TODO Fix execvp (renvoie tjr -1)
            err = execvp(toexec[0], toexec);
            
            char* temp = malloc(20);
            sprintf(temp, "%ld:%d\n", time(NULL), err);
            write(fd_task, temp, strlen(temp));
            close(fd_task);
         }

         close(fd_task);
         close(task_stdout);
         close(task_stderr);
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

         first_id_available++;
         break;
      }
   }
   return 0;
}

int server_terminate(int fd_req, char* rep_pipe) {
   close(fd_req);
   uint16_t reptype = htobe16(SERVER_REPLY_OK);
   int fd_rep = open(rep_pipe, O_WRONLY);
   write(fd_rep, &reptype, sizeof(uint16_t));
   close(fd_rep);

   // kill les taches en cours
   char taskdir[50];
   char* username = getenv("USER");
   sprintf(taskdir, "/tmp/%s/saturnd/tasks", username);
   DIR* dirp = opendir(taskdir);
   struct dirent* entry;
   char name[50]; 
   while ((entry = readdir(dirp))) {
      if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
         uint64_t task_id = atoi(entry->d_name);
         sprintf(name, "/tmp/%s/saturnd/tasks/%ld/pid", username, task_id);
         int fd_task = open(name, O_RDONLY, S_IRWXU);
         char* buffer = malloc(MAX_BUFFER);
         int nb_read = read(fd_task, buffer, MAX_BUFFER);
         if (nb_read > 0) {
            int pid = atoi(buffer);
            kill(pid, SIGKILL);
         }
         // TODO Supprimer pid pour le récréer au prochain démarrage
      }
   }
   closedir(dirp);
   return EXIT_SUCCESS;
}

int server_remove(int fd_req, char* rep_pipe) {
   uint64_t task_id;
   read(fd_req, &task_id, sizeof(uint64_t));
   task_id = htobe64(task_id);
   close(fd_req);
   
   int killed = 0;
   char taskdir[50];
   char* username = getenv("USER");

   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/command", username, task_id);
   remove(taskdir);
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/stdout", username, task_id);
   remove(taskdir);
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/stderr", username, task_id);
   remove(taskdir);
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/exitcodes", username, task_id);
   remove(taskdir);
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/pid", username, task_id);
   int fd_task = open(taskdir, O_RDONLY, S_IRWXU);
   char* buffer = malloc(MAX_BUFFER);
   int nb_read = read(fd_task, buffer, MAX_BUFFER);
   if (nb_read > 0) {
      int pid = atoi(buffer);
      kill(pid, SIGKILL);
      killed = 1;
   }
   remove(taskdir);
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld", username, task_id);
   rmdir(taskdir);

   int fd_rep = open(rep_pipe, O_WRONLY); 
   uint16_t reptype;
   if (killed) {
      reptype = htobe16(SERVER_REPLY_OK);
      write(fd_rep, &reptype, sizeof(uint16_t));
   } else {
      reptype = htobe16(SERVER_REPLY_ERROR);
      write(fd_rep, &reptype, sizeof(uint16_t));
      uint16_t errtype = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
      write(fd_rep, &errtype, sizeof(uint16_t));
   }
   close(fd_rep);

   return 0;  
}

int server_times_exit(int fd_req, char* rep_pipe) {
   uint64_t task_id;
   read(fd_req, &task_id, sizeof(uint64_t));
   task_id = htobe64(task_id);
   close(fd_req);

   char taskdir[50];
   char* username = getenv("USER");
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/exitcodes", username, task_id);
   int fd_task = open(taskdir, O_RDONLY, S_IRWXU);
   int fd_rep = open(rep_pipe, O_WRONLY, S_IRWXU);
   uint16_t reptype ;
   if (fd_task == -1) {
      reptype = htobe16(SERVER_REPLY_ERROR);
      write(fd_rep, &reptype, sizeof(uint16_t));
      uint16_t errtype = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
      write(fd_rep, &errtype, sizeof(uint16_t));
   } else {
      reptype = htobe16(SERVER_REPLY_OK);
      write(fd_rep, &reptype, sizeof(uint16_t));
      char* buffer = malloc(MAX_BUFFER);
      uint32_t i = 0;
      while (1) {
         int nb_read = read(fd_task, buffer, MAX_BUFFER);
         if (nb_read <= 0) {
            break;
         }
         char* token = strtok(buffer, "\n");
         while (token != NULL) {
            i++;
            token = strtok(NULL, "\n");
         }
      }
      uint32_t nb_runs = htobe32(i);
      write(fd_rep, &nb_runs, sizeof(uint32_t));
      while (1) {
         free(buffer);
         buffer = malloc(MAX_BUFFER);
         int nb_read = read(fd_task, buffer, MAX_BUFFER);
         if (nb_read <= 0) {
            break;
         }
         char* token = strtok(buffer, "\n");
         while (token != NULL) {
            char* token2;
            char* rest = token;
            token2 = strtok_r(rest, ":", &rest);
            int64_t btime = htobe64(atoi(token2));
            token2 = strtok_r(rest, ":", &rest);
            uint16_t exitcode = htobe16(atoi(token2));
            write(fd_rep, &btime, sizeof(int64_t));
            write(fd_rep, &exitcode, sizeof(uint16_t));
            token = strtok(NULL, "\n");
         }
      }
   }
   close(fd_rep);
   return 0;     
}

int server_stdout(int fd_req, char* rep_pipe) {
   uint64_t task_id;
   read(fd_req, &task_id, sizeof(uint64_t));
   task_id = htobe64(task_id);
   close(fd_req);

   char taskdir[50];
   char* username = getenv("USER");
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/stdout", username, task_id);
   int fd_task = open(taskdir, O_RDONLY, S_IRWXU);
   int fd_rep = open(rep_pipe, O_WRONLY, S_IRWXU);
   uint16_t reptype;
   uint16_t errtype;
   if (fd_task == -1) {
      reptype = htobe16(SERVER_REPLY_ERROR);
      write(fd_rep, &reptype, sizeof(uint16_t));
      errtype = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
      write(fd_rep, &errtype, sizeof(uint16_t));
      close(fd_rep);
   } else {
      struct stat st;
      stat(taskdir, &st);
      if (st.st_size == 0) {
         reptype = htobe16(SERVER_REPLY_ERROR);
         write(fd_rep, &reptype, sizeof(uint16_t));
         uint16_t errtype;
         errtype = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
         write(fd_rep, &errtype, sizeof(uint16_t));
         close(fd_rep);
      } else {
         reptype = htobe16(SERVER_REPLY_OK);
         write(fd_rep, &reptype, sizeof(uint16_t));
         char* buffer = malloc(MAX_BUFFER);
         while (1) {
            int nb_read = read(fd_task, buffer, MAX_BUFFER);
            if (nb_read <= 0) {
               break;
            }
            write(fd_rep, buffer, nb_read);
         }
         close(fd_task);
      }
   }
   close(fd_rep);
   return 0;
}

int server_stderr(int fd_req, char* rep_pipe) {
   uint64_t task_id;
   read(fd_req, &task_id, sizeof(uint64_t));
   task_id = htobe64(task_id);
   close(fd_req);

   char taskdir[50];
   char* username = getenv("USER");
   sprintf(taskdir, "/tmp/%s/saturnd/tasks/%ld/stderr", username, task_id);
   int fd_task = open(taskdir, O_RDONLY, S_IRWXU);
   int fd_rep = open(rep_pipe, O_WRONLY);
   uint16_t reptype ;
   uint16_t errtype;
   if (fd_task == -1) {
      reptype = htobe16(SERVER_REPLY_ERROR);
      write(fd_rep, &reptype, sizeof(uint16_t));
      errtype = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
      write(fd_rep, &errtype, sizeof(uint16_t));
      close(fd_rep);
   } else {
      struct stat st;
      stat(taskdir, &st);
      if (st.st_size == 0) {
         reptype = htobe16(SERVER_REPLY_ERROR);
         write(fd_rep, &reptype, sizeof(uint16_t));
         uint16_t errtype;
         errtype = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
         write(fd_rep, &errtype, sizeof(uint16_t));
         close(fd_rep);
      } else {
         reptype = htobe16(SERVER_REPLY_OK);
         write(fd_rep, &reptype, sizeof(uint16_t));
         char* buffer = malloc(MAX_BUFFER);
         while (1) {
            int nb_read = read(fd_task, buffer, MAX_BUFFER);
            if (nb_read <= 0) {
               break;
            }
            write(fd_rep, buffer, nb_read);
         }
         close(fd_task);
      }
   }
   close(fd_rep);
   return 0;
}