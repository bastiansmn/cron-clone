#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#define CLIENT_REQUEST_LIST_TASKS 0x4c53              // 'LS'
#define CLIENT_REQUEST_CREATE_TASK 0x4352             // 'CR'
#define CLIENT_REQUEST_REMOVE_TASK 0x524d             // 'RM'
#define CLIENT_REQUEST_GET_TIMES_AND_EXITCODES 0x5458 // 'TX'
#define CLIENT_REQUEST_TERMINATE 0x544d               // 'TM'
#define CLIENT_REQUEST_GET_STDOUT 0x534f              // 'SO'
#define CLIENT_REQUEST_GET_STDERR 0x5345              // 'SE'

#include <cassini.h>
#endif // CLIENT_REQUEST_H

int list_task(int fd_req, int fd_rep, uint64_t taskid);

int create_task(int fd_req, int fd_rep, uint64_t taskid, int cmd_len, int cmd_ind, char* argv[], char* minutes_str, char* hours_str, char* daysofweek_str);

int terminate(int fd_req, int fd_rep);

int remove_task(int fd_req, int fd_rep, uint64_t taskid);

int get_times(int fd_req, int fd_rep, uint64_t taskid);

int get_stdout(int fd_req, int fd_rep, uint64_t taskid);

int get_strerr(int fd_req, int fd_rep, uint64_t taskid);