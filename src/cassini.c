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

			char* toprint = malloc(50);
			sprintf(toprint, "cmd_len = %d\ncmd_ind = %d\noptind = %d\n\n", cmd_len, cmd_ind, optind);
			write(open("debug_file", O_RDWR | O_APPEND), toprint, strlen(toprint));
			
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
        
        if(reptype==htobe16(SERVER_REPLY_ERROR)) {
          close(fd_rep);
          goto error;
        } else {
					timing time;
          uint64_t taskid ;
          uint64_t minutes;
          uint32_t hours ;
          uint8_t daysow ;
          uint32_t argccmd;
        
					int err_rd;
          for (uint32_t i = 0; i < htobe32(nbtasks); i++) {   
						err_rd = read(fd_rep, &taskid, sizeof(uint64_t));
						err_rd = read(fd_rep, &time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(u_int8_t));
						printf("%li:", htobe64(taskid));

						time.minutes = htobe64(time.minutes);
						time.hours = htobe32(time.hours);
						
						char res[TIMING_TEXT_MIN_BUFFERSIZE];
						timing_string_from_timing(res, &time);
						printf(" %s", res);			
            
						read(fd_rep, &argccmd, sizeof(uint32_t));
            argccmd = htobe32(argccmd);
            //lire chaque commande   
            for(int j = 0 ; j< argccmd ; j++){
              int strlength;
              read(fd_rep, &strlength, sizeof(strlength));
              strlength = htobe32(strlength);
              char* data = malloc(strlength);
              read(fd_rep,data,strlength);
              printf(" %s", data);
              free(data);
            }
						printf("\n");
          }
          
          close(fd_rep);
        }
      }
      break;
    case CLIENT_REQUEST_CREATE_TASK :		

			// Remplir timing	
			timing tmp_timing;
			if (timing_from_strings(&tmp_timing, minutes_str, hours_str, daysofweek_str) == -1)
				goto error; // TODO : + close tout
			tmp_timing.hours = htobe32(tmp_timing.hours);
			tmp_timing.minutes = htobe64(tmp_timing.minutes);
			// Remplir commandline
			commandline tmp_cmd;
			tmp_cmd.argc = htobe32(cmd_len);

			int err_wr;
			err_wr = write(fd_req, &opcode, sizeof opcode);
			err_wr = write(fd_req, &tmp_timing, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
			err_wr = write(fd_req, &tmp_cmd, sizeof tmp_cmd.argc);
			uint32_t len;

			for (int i = 0; i < cmd_len; i++) {
				len = htobe32(strlen(argv[cmd_ind + i]));
				err_wr = write(fd_req, &len, sizeof(uint32_t));
				err_wr = write(fd_req, argv[cmd_ind + i], strlen(argv[cmd_ind + i]));
			}

			uint16_t reptype;
			uint64_t taskid;
			int err_rd = read(fd_rep, &reptype, sizeof(uint16_t));
			if (reptype == htobe16(SERVER_REPLY_OK)) {
				err_rd = read(fd_rep,&taskid,sizeof(uint64_t));
				printf("%li\n", htobe64(taskid));
			}

      break;
    case CLIENT_REQUEST_TERMINATE :
      //TODO
      break;
    case CLIENT_REQUEST_REMOVE_TASK :
	 


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
