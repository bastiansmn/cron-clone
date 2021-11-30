#include "client-request.h"
#include "timing.h"
#include "stringc.h"
#include "command-line.h"

void write_time(timing* time, int writer){
  write(writer,&time->daysofweek,sizeof(time->daysofweek));
  write(writer,&time->hours,sizeof(time->hours));
  write(writer,&time->minutes,sizeof(time->minutes));
}

void write_string(struct stringc* str ,int writer){
  write(writer,&str->L,sizeof(str->L));
  write(writer,&str->val,sizeof(str->val));
}

void write_cmd(commandline cmd , int writer){
  write_string(cmd.argv,writer);
  write(writer,&cmd.argc, sizeof(cmd.argc));
}