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
  int cmd_len;
  int cmd_ind;

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

      int ind_temp = optind;
      while (*argv[ind_temp] == '-') {
        ind_temp += 2;
      }
      cmd_ind = ind_temp;
      cmd_len = argc - cmd_ind;         
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

  int fd_req, fd_rep;
  if (pipes_directory == NULL) {
    char req_pipe[50];
    char rep_pipe[50];
    char* username = getenv("USER");
    sprintf(req_pipe, "/tmp/%s/saturnd/pipes/request", username);
    sprintf(rep_pipe, "/tmp/%s/saturnd/pipes/reply", username);
    
    fd_req = open(req_pipe, O_WRONLY);
    fd_rep = open(rep_pipe, O_RDONLY | O_NONBLOCK);
    if (fd_rep == -1 || fd_req == -1) {
      perror("open");
      exit (EXIT_FAILURE);
    }
  } else {
    char* req_pipe = strdup(pipes_directory);
    strcat(req_pipe, strdup("/saturnd-request-pipe"));
    char* rep_pipe = strdup(pipes_directory);
    strcat(rep_pipe, strdup("/saturnd-reply-pipe"));
    fd_req = open(req_pipe, O_WRONLY);
    fd_rep = open(rep_pipe, O_RDONLY | O_NONBLOCK);
    if (fd_rep == -1 || fd_req == -1) {
      perror("open");
      exit (EXIT_FAILURE);
    }
  }

  int res;
  switch (operation) {	
    case CLIENT_REQUEST_LIST_TASKS : {
      res = list_task(fd_req, fd_rep, taskid);
      if (res != 0)
        goto error;
      break;
		}
    case CLIENT_REQUEST_CREATE_TASK :	{
      res = create_task(fd_req, fd_rep, taskid, cmd_len, cmd_ind, argv, minutes_str, hours_str, daysofweek_str);
      if (res != 0)
        goto error;
      break;
		}
    case CLIENT_REQUEST_TERMINATE : {
      res = terminate(fd_req, fd_rep);
      if (res != 0)
        goto error;
      break;
		}
		case CLIENT_REQUEST_REMOVE_TASK : {
      res = remove_task(fd_req, fd_rep, taskid);
      if (res != 0)
        goto error;
      break;
		}
    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES : {
      res = get_times(fd_req, fd_rep, taskid);
      if (res != 0)
        goto error;
      break;
		}
    case CLIENT_REQUEST_GET_STDOUT: {
      res = get_stdout(fd_req, fd_rep, taskid);
      if (res != 0)
        goto error;
      break;
		}
    case CLIENT_REQUEST_GET_STDERR: {
      res = get_strerr(fd_req, fd_rep, taskid);
      if (res != 0)
        goto error;
      break;
		}
  }
  // --------
  
  return EXIT_SUCCESS;

 error:
  // TODO : Free tous les éléments (en particulier les strings) qui ont été malloc (ou autre fonctions)
  if (errno != 0) perror("main");
  close(fd_rep);
  close(fd_req);
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
