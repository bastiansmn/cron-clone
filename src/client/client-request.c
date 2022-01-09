#include "client-request.h"


void close_pipes(int req, int rep) {
  close(req); close(rep);
}

int list_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_LIST_TASKS);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  int req = write(fd_req, &opcode, sizeof(opcode));
  if (req<0) {
    close(fd_req);
  } else {
    int fd_rep = open(rep_pipe_dir,O_RDONLY);
    uint16_t reptype;
    uint32_t nbtasks;
    read (fd_rep, &reptype, sizeof(reptype));
    read (fd_rep, &nbtasks, sizeof(nbtasks));
    
    if(reptype==htobe16(SERVER_REPLY_ERROR)) {
      close_pipes(fd_req, fd_req);
      return 1;
    } else {
      timing time;
      uint32_t argccmd;

      for (uint32_t i = 0; i < htobe32(nbtasks); i++) {   
        read(fd_rep, &taskid, sizeof(uint64_t));
        read(fd_rep, &time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(u_int8_t));
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
          char* data = malloc(strlength+1);
          read(fd_rep, data, strlength);
          data[strlength] = '\0';
          printf(" %s", data);
          free(data);
        }
        printf("\n");
      }
    }
  }
  close_pipes(fd_req, fd_req);
  return 0;
}

int create_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid, int cmd_len, int cmd_ind, char* argv[], char* minutes_str, char* hours_str, char* daysofweek_str) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_CREATE_TASK);
  // Remplir timing	
  timing tmp_timing;
  if (timing_from_strings(&tmp_timing, minutes_str, hours_str, daysofweek_str) == -1)
    return 1; // TODO : + close tout
  tmp_timing.hours = htobe32(tmp_timing.hours);
  tmp_timing.minutes = htobe64(tmp_timing.minutes);
  // Remplir commandline
  commandline tmp_cmd;
  tmp_cmd.argc = htobe32(cmd_len);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof opcode);
  write(fd_req, &tmp_timing, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
  write(fd_req, &tmp_cmd, sizeof tmp_cmd.argc);
  uint32_t len;

  for (int i = 0; i < cmd_len; i++) {
    len = htobe32(strlen(argv[cmd_ind + i]));
    write(fd_req, &len, sizeof(uint32_t));
    write(fd_req, argv[cmd_ind + i], strlen(argv[cmd_ind + i]));
  }

  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype;
  read(fd_rep, &reptype, sizeof(uint16_t));
  if (reptype == htobe16(SERVER_REPLY_OK)) {
    read(fd_rep,&taskid,sizeof(uint64_t));
    printf("%li\n", htobe64(taskid));
  }
  close_pipes(fd_req, fd_req);
  return 0;
}

int terminate(char* req_pipe_dir, char* rep_pipe_dir){
  uint16_t opcode = htobe16(CLIENT_REQUEST_TERMINATE);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof(opcode));
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype = htobe16(SERVER_REPLY_OK);
  read(fd_rep, &reptype, sizeof(uint16_t));
  close_pipes(fd_req, fd_rep);
  return 0;
}

int remove_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  taskid = htobe64(taskid);
  uint16_t opcode = htobe16(CLIENT_REQUEST_REMOVE_TASK);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req,&opcode,sizeof(opcode));
  write(fd_req,&taskid,sizeof(taskid)); 
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype = htobe16(SERVER_REPLY_OK);
  read(fd_rep, &reptype, sizeof(uint16_t));
  close_pipes(fd_req, fd_rep);
  return 0;
}

int get_times(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t reptype;
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_TIMES_AND_EXITCODES);
  taskid = htobe64(taskid);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof(opcode));
  write(fd_req, &taskid, sizeof(taskid)); 
  uint32_t nbruns ;
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  read(fd_rep, &reptype, sizeof(reptype));
  if(reptype== htobe16(SERVER_REPLY_OK)) {
    read(fd_rep, &nbruns, sizeof(nbruns));
    nbruns = htobe32(nbruns);
    int64_t time;
    int16_t exitcode;
    time_t rawtime ;
    char buf[80];
    for (uint32_t i = 0; i < nbruns; i++) {   
      read(fd_rep, &time, sizeof(int64_t));
      read(fd_rep, &exitcode, sizeof(int16_t));
      rawtime = htobe64(time);
      struct tm ts;
      ts = *localtime((&rawtime));
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &ts);
      printf("%s", buf);
      printf("%li\n",(taskid));
    }
  }
  else {
    uint16_t errcode ;
    read(fd_rep, &errcode , sizeof(errcode));
    close_pipes(fd_req, fd_req);
    return 1;
  }
  close_pipes(fd_req, fd_req);
  return 0;
}

int get_stdout(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_STDOUT);
  uint16_t reptype;

  taskid = htobe64(taskid);

  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req,&opcode,sizeof(opcode));
  write(fd_req,&taskid,sizeof(taskid));
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  read(fd_rep,&reptype,sizeof(reptype));
  if(reptype==htobe16(SERVER_REPLY_OK)){
    uint32_t L ;
    char* res = malloc(L);
    read(fd_rep,&L,sizeof(L));
    read(fd_rep,res,htobe32(L));
    printf("%s",res);
  }
  else{
    uint16_t errcode ;
    read(fd_rep, &errcode , sizeof(errcode));
    close_pipes(fd_req, fd_req);
    return 1;
  }
  close_pipes(fd_req, fd_req);
  return 0;
}

int get_strerr(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_STDERR);
  taskid = htobe64(taskid);
  uint16_t reptype;
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req,&opcode,sizeof(opcode));
  write(fd_req,&taskid,sizeof(taskid));
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  read(fd_rep,&reptype,sizeof(reptype));
  if(reptype==htobe16(SERVER_REPLY_OK)){    
    uint32_t strlength;
    read(fd_rep, &strlength, sizeof(strlength));
    strlength = htobe32(strlength);
    char* data = malloc(strlength+1);
    read(fd_rep,data,strlength);
    data[strlength]= '\0';
    printf("%s", data);
    free(data);
  }
  else{
    uint16_t errcode ;
    read(fd_rep, &errcode , sizeof(errcode));
    close_pipes(fd_req, fd_req);
    return 1;
  }
  printf("\n");
  close_pipes(fd_req, fd_req);
  return 0;
}