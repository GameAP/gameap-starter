#ifndef PROC_H
#define PROC_H

#define PROC_H

#include <map>
#include <list>

// typedef unsigned long pid_t;

pid_t find_pid_by_path(const char *path);
void killall(const char *path);
unsigned int count_proc_in_path(const char *path);

std::map<int, std::list<int>> process_childs();
void killtree(pid_t pid);

#endif
