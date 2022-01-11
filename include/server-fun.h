#ifndef SERVER_FUN_H
#define SERVER_FUN_H

#include <saturnd.h>

#define MAX_BUFFER 256

#endif

int server_list_task(int fd_req, char* rep_pipe);

int server_create_task(int fd_req, char* rep_pipe, DIR* tasksdir, uint64_t first_id_available);

int server_terminate(int fd_req, char* rep_pipe);

int server_remove(int fd_req, char* rep_pipe);

int server_times_exit(int fd_req, char* rep_pipe);

int server_stdout(int fd_req, char* rep_pipe);

int server_stderr(int fd_req, char* rep_pipe);