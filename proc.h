#ifndef PROC_H
#define PROC_H

#define PROC_H

#include <map>
#include <list>

#ifdef _WIN32
    typedef unsigned int uint;
    typedef unsigned short ushort;
    typedef unsigned long ulong;
    typedef int pid_t;
#endif

pid_t find_pid_by_path(const char *path);
void killall(const char *path);
unsigned int count_proc_in_path(const char *path);

std::map<int, std::list<int>> process_childs();
void killtree(pid_t pid);

#endif
