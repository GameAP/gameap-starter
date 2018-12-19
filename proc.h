#ifndef PROC_H
#define PROC_H

#define PROC_H

// typedef unsigned long pid_t;

pid_t find_pid_by_path(const char *path);
void killall(const char *path);
unsigned int count_proc_in_path(const char *path);

#endif
