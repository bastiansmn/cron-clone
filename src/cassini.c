#include "cassini.h"

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

int main(int argc, char * argv[]) {
  errno = 0;
  
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;
  
  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;
  
  int opt;
  char * strtoull_endp;
  int index_of_command;
  int len_of_command;

  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
    int ind_temp = optind - 1;
    while (*argv[ind_temp] == '-') {
      ind_temp += 2;
    }
    index_of_command = ind_temp;
    len_of_command = argc - index_of_command;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }
  // --------
  // | TODO |
  // --------	
  

  int fd_req, fd_rep;
  if (pipes_directory == NULL) {
    char req_pipe[50];
    char rep_pipe[50];
    char* username = getenv("USER");
    sprintf(req_pipe, "/tmp/%s/saturnd/pipes/request", username);
    sprintf(rep_pipe, "/tmp/%s/saturnd/pipes/reply", username);
    
    fd_req = open(req_pipe, O_WRONLY);
    fd_rep = open(rep_pipe, O_RDONLY);
  } else {
    char* req_pipe = strdup(pipes_directory);
    strcat(req_pipe, strdup("/saturnd-request-pipe"));
    char* rep_pipe = strdup(pipes_directory);
    strcat(rep_pipe, strdup("/saturnd-reply-pipe"));
    fd_req = open(req_pipe, O_WRONLY);
    fd_rep = open(rep_pipe, O_RDONLY);
  }
  uint16_t opcode = htobe16(operation);
  switch (operation) {	
    case CLIENT_REQUEST_LIST_TASKS :
      int req = write(fd_req, &opcode, sizeof(opcode));
      if (req<0) {
        close(fd_req);
      } else {
        uint16_t reptype ;
        uint32_t nbtasks ;
        read (fd_rep,&reptype,sizeof(reptype));
        read (fd_rep,&nbtasks,sizeof(nbtasks));
        
        if(htobe16(reptype)==SERVER_REPLY_ERROR) {
          close(fd_rep);
          goto error;
        } else {
          uint64_t id ;
          uint64_t minutes;
          uint32_t hours ;
          uint8_t days ;
          uint32_t argccmd;
          struct stringc* argvcmd ;
          if (htobe32(nbtasks)<0) {
            close(fd_rep);
            goto error;
          } else {
            for (uint32_t i = 0 ; i< htobe32(nbtasks);i++) {
              printf("%li",htobe16(reptype));
              
              printf("%li",htobe64(read(fd_rep,&id,sizeof(id))));

              printf("%li",htobe64(read(fd_rep,&minutes,sizeof(minutes))));

              printf("%li",htobe32(read(fd_rep,&hours,sizeof(hours))));

              printf("%li",read(fd_rep,&days,sizeof(days)));

              read(fd_rep,&argccmd,sizeof(argccmd));
              argccmd = htobe32(argccmd) ;
              
              //lire chaque commande   
              for(unsigned int i = 0 ; i<argccmd ; i++){
                int strlength ;
                read(fd_rep,&strlength ,sizeof(strlength));
                strlength = htobe32(strlength);
                char* data = malloc(strlength);
                read(fd_rep,data,strlength);
                printf("%s",data);
                free(data);
              }
            }
          }
          close(fd_rep);
        }
      }
      break;
    case CLIENT_REQUEST_CREATE_TASK :
      timing time;
      if (timing_from_strings(&time, minutes_str, hours_str, daysofweek_str) == -1) {
        goto error;
      }
      time.hours = htobe32(time.hours);
      time.minutes = htobe64(time.minutes);
      // TODO : Mieux parse cmd
      commandline cmd;
      cmd.argc = len_of_command;
      cmd.argv = argv[index_of_command];
      // TODO : Utiliser des memmove
      int err_write;
      err_write = write(fd_req, &opcode, sizeof(opcode));
      err_write = write(fd_req, &time, sizeof(time));
      err_write = write(fd_req, &cmd, sizeof(cmd));
      break;
    case CLIENT_REQUEST_TERMINATE :
      //TODO
      break;
    case CLIENT_REQUEST_REMOVE_TASK :
      uint16_t reptype ;
      uint16_t taskid ;


      break;
    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES :
      //TODO
      break;
    case CLIENT_REQUEST_GET_STDOUT:
      //TODO
      break;
    case CLIENT_REQUEST_GET_STDERR:
      //TODO
      break;
  }
  // --------
  
  return EXIT_SUCCESS;

 error:
  // TODO : Free tous les éléments (en particulier les strings) qui ont été malloc (ou autre fonctions)
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}

void debug_f(char* toprint) {
  write(open("debug_file", O_RDWR | O_CREAT | O_TRUNC), toprint, strlen(toprint));
}
