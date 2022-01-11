#include "client-request.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif


int list_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_LIST_TASKS);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  int req = write(fd_req, &opcode, sizeof(opcode));
  close(fd_req);
  int fd_rep = open(rep_pipe_dir,O_RDONLY);
  if (req<0) {
    perror("write");
    return -1;
  } else {
    uint16_t reptype;
    uint32_t nbtasks;
    read (fd_rep, &reptype, sizeof(reptype));
    read (fd_rep, &nbtasks, sizeof(nbtasks));
    reptype = htobe16(reptype);
    nbtasks = htobe32(nbtasks);

    if(reptype!=SERVER_REPLY_OK) {
      close(fd_rep);
      return 1;
    } else {
      timing time;
      uint32_t argccmd;

      for (uint32_t i = 0; i < nbtasks; i++) {   
        read(fd_rep, &taskid, sizeof(uint64_t));
        read(fd_rep, &time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
        printf("%lu:", htobe64(taskid));

        time.minutes = htobe64(time.minutes);
        time.hours = htobe32(time.hours);
        
        char res[TIMING_TEXT_MIN_BUFFERSIZE];
        int len = timing_string_from_timing(res, &time);
        res[len] = '\0';
        printf(" %s", res);			
    
        read(fd_rep, &argccmd, sizeof(uint32_t));
        argccmd = htobe32(argccmd);
        //lire chaque commande   
        for(int j = 0 ; j < argccmd ; j++){
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
  close(fd_rep);
  return 0;
}

int create_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid, int cmd_len, int cmd_ind, char* argv[], char* minutes_str, char* hours_str, char* daysofweek_str) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_CREATE_TASK);
  // Remplir timing	
  timing tmp_timing;
  if (timing_from_strings(&tmp_timing, minutes_str, hours_str, daysofweek_str) == -1) {
    return 1; // TODO : + close tout
  }
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
  close(fd_req);

  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype;
  read(fd_rep, &reptype, sizeof(uint16_t));
  if (reptype == htobe16(SERVER_REPLY_OK)) {
    read(fd_rep,&taskid,sizeof(uint64_t));
    printf("%li\n", htobe64(taskid));
  }
  close(fd_rep);
  return 0;
}

int terminate(char* req_pipe_dir, char* rep_pipe_dir){
  uint16_t opcode = htobe16(CLIENT_REQUEST_TERMINATE);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof(opcode));
  close(fd_req);
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype = htobe16(SERVER_REPLY_OK);
  read(fd_rep, &reptype, sizeof(uint16_t));
  close(fd_rep);
  return 0;
}

int remove_task(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  taskid = htobe64(taskid);
  uint16_t opcode = htobe16(CLIENT_REQUEST_REMOVE_TASK);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req,&opcode,sizeof(opcode));
  write(fd_req,&taskid,sizeof(taskid)); 
  close(fd_req);
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  uint16_t reptype;
  read(fd_rep, &reptype, sizeof(uint16_t));
  reptype = htobe16(reptype);
  if (reptype != SERVER_REPLY_OK) {
    uint16_t errtype;
    read(fd_rep, &errtype, sizeof(uint16_t));
    errtype = htobe16(errtype);
    if (errtype == SERVER_REPLY_ERROR_NOT_FOUND)
      printf("SERVER_REPLY_ERROR_NOT_FOUND\n");
  }
  close(fd_rep);
  return 0;
}

int get_times(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t reptype;
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_TIMES_AND_EXITCODES);
  taskid = htobe64(taskid);
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof(opcode));
  write(fd_req, &taskid, sizeof(taskid)); 
  close(fd_req);
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
      close(fd_rep);
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
    close(fd_rep);
    return 1;
  }
  return 0;
}

int get_stdout(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_STDOUT);
  uint16_t reptype;

  taskid = htobe64(taskid);

  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req, &opcode, sizeof(opcode));
  write(fd_req, &taskid, sizeof(taskid));
  close(fd_req);
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  read(fd_rep,&reptype,sizeof(reptype));
  if (reptype==htobe16(SERVER_REPLY_OK)) {
    char* buffer = malloc(2);
    int rd;
    while ((rd = read(fd_rep, buffer, 2)) > 0)
      printf("%s", buffer);
    printf("\n");
    close(fd_rep);
  } else {
    uint16_t errcode ;
    read(fd_rep, &errcode , sizeof(errcode));
    close(fd_rep);
    return 1;
  }
  return 0;
}

int get_strerr(char* req_pipe_dir, char* rep_pipe_dir, uint64_t taskid) {
  uint16_t opcode = htobe16(CLIENT_REQUEST_GET_STDERR);
  taskid = htobe64(taskid);
  uint16_t reptype;
  int fd_req = open(req_pipe_dir, O_WRONLY);
  write(fd_req,&opcode,sizeof(opcode));
  write(fd_req,&taskid,sizeof(taskid));
  close(fd_req);
  int fd_rep = open(rep_pipe_dir, O_RDONLY);
  read(fd_rep,&reptype,sizeof(reptype));
  if(reptype==htobe16(SERVER_REPLY_OK)){    
    uint32_t strlength;
    read(fd_rep, &strlength, sizeof(strlength));
    strlength = htobe32(strlength);
    char* data = malloc(strlength+1);
    read(fd_rep,data,strlength);
    close(fd_rep);
    data[strlength]= '\0';
    printf("%s", data);
    free(data);
  } else {
    uint16_t errcode ;
    read(fd_rep, &errcode , sizeof(errcode));
    close(fd_rep);
    return 1;
  }
  printf("\n");
  return 0;
}