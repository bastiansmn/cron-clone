#ifndef SERVER_FUN_H
#define SERVER_FUN_H

#include <saturnd.h>

#define MAX_BUFFER 128

#endif

int server_list_task();

int server_create_task(int fd_req, char* rep_pipe, DIR* tasksdir, uint64_t first_id_available, int pid_run_fd, char* username);

int server_terminate(int fd_req, char* rep_pipe, char* pid_dir);

int server_remove();

int server_times_exit();

int server_stdout();

int server_stderr();