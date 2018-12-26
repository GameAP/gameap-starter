#ifdef __linux__
#include <unistd.h>
#include <glob.h>
#include <dirent.h>
#include <signal.h>
#endif

#ifdef _WIN32

#include <windows.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#endif

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <stdlib.h>

#include "proc.h"

#ifdef __linux__

// ---------------------------------------------------------------------

pid_t find_pid_by_path(const char *path)
{
    pid_t pid = -1;
    glob_t pglob;

    if (glob("/proc/*/exe", 0, nullptr, &pglob) != 0)
        return -1;

    char buff[PATH_MAX];
    char pbuff[PATH_MAX];

    for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
        ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

        if (len == -1) continue;

        buff[len] = '\0';

        char * lslash = strrchr(buff, '/');
        memcpy(pbuff, buff, (lslash - buff));
        pbuff[(lslash - buff)] = '\0';

        if (strcmp(path, pbuff) == 0) {
            pid = (pid_t)atoi(pglob.gl_pathv[i] + strlen("/proc/"));

            if (pid == getpid()) continue;
            else break;
        }
    }

    globfree(&pglob);

    if (pid == -1) {
        if (glob("/proc/*/cwd", 0, nullptr, &pglob) != 0)
            return -1;

        for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
            ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

            if (len == -1) continue;

            buff[len] = '\0';

            if (strcmp(path, buff) == 0) {
                pid = (pid_t)atoi(pglob.gl_pathv[i] + strlen("/proc/"));

                if (pid == getpid()) continue;
                else break;
            }
        }
    }

    globfree(&pglob);

    return pid;
}

// ---------------------------------------------------------------------

unsigned int count_proc_in_path(const char *path)
{
    pid_t pid = -1;
    glob_t pglob;

    if (glob("/proc/*/exe", 0, nullptr, &pglob) != 0)
        return -1;

    char buff[PATH_MAX];
    char pbuff[PATH_MAX];

    unsigned int pcount = 0;

    for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
        ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

        if (len == -1) continue;

        buff[len] = '\0';

        char * lslash = strrchr(buff, '/');
        memcpy(pbuff, buff, (lslash - buff));
        pbuff[(lslash - buff)] = '\0';

        if (strcmp(path, pbuff) == 0) {
            pcount++;
        }
    }

    globfree(&pglob);

    if (pid == -1) {
        if (glob("/proc/*/cwd", 0, nullptr, &pglob) != 0)
            return -1;

        for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
            ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

            if (len == -1) continue;

            buff[len] = '\0';

            if (strcmp(path, buff) == 0) {
                pcount++;
            }
        }
    }

    globfree(&pglob);

    return pcount;
}

// ---------------------------------------------------------------------

void killall(const char *path)
{
    pid_t pid = -1;
    glob_t pglob;

    if (glob("/proc/*/exe", 0, nullptr, &pglob) != 0)
        return;

    char buff[PATH_MAX];
    char pbuff[PATH_MAX];

    for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
        ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

        if (len == -1) continue;

        buff[len] = '\0';

        char * lslash = strrchr(buff, '/');
        memcpy(pbuff, buff, (lslash - buff));
        pbuff[(lslash - buff)] = '\0';

        if (strcmp(path, pbuff) == 0) {
            pid = (pid_t)atoi(pglob.gl_pathv[i] + strlen("/proc/"));

            if (pid == getpid()) continue;

            std::cout << "Killing: " << pid << std::endl;

            if (kill(pid, SIGTERM) != 0) {
                std::cerr << "Stop error (killall): " << strerror(errno) << std::endl;
            }

            break;
        }
    }

    globfree(&pglob);


    //if (glob("/proc/*/cwd", 0, nullptr, &pglob) != 0)
    //    return;

    /*
    for (unsigned i = 0; i < pglob.gl_pathc; ++i) {
        ssize_t len = ::readlink(pglob.gl_pathv[i], buff, sizeof(buff)-1);

        if (len == -1) continue;

        buff[len] = '\0';

        if (strcmp(path, buff) == 0) {
            pid = (pid_t)atoi(pglob.gl_pathv[i] + strlen("/proc/"));

            if (pid == getpid()) continue;

            std::cout << "Killing: " << pid << std::endl;

            if (kill(pid, SIGTERM) != 0) {
                std::cerr << "Stop error (killall): " << strerror(errno) << std::endl;
            }
        }
    }
    */

    globfree(&pglob);
}

#elif _WIN32

std::vector<std::string> explode(std::string delimiter, std::string inputstring){
    std::vector<std::string> explodes;

    inputstring.append(delimiter);

    while(inputstring.find(delimiter)!=std::string::npos){
        explodes.push_back(inputstring.substr(0, inputstring.find(delimiter)));
        inputstring.erase(inputstring.begin(), inputstring.begin()+inputstring.find(delimiter)+delimiter.size());
    }

    return explodes;
}

std::string exec(std::string cmd) {
	std::cout << "EX CMD: " << cmd << std::endl;
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != nullptr)
            result += buffer;
    }
    return result;
}

pid_t find_pid_by_path(const char *path)
{
    pid_t pid = -1;

	std::string spath = std::string(path);

	boost::replace_all(spath, "\\", "\\\\");
    std::string result = exec(str(boost::format("wmic process where \"executablepath like '%s%%'\" get processid") % spath));
	std::cout << "result: " << result << std::endl;
	std::vector<std::string> expl = explode("\n", result);

    for (int i = 1; i < expl.size(); i++) {
		std::cout << "find_pid_by_path EXPL: " << expl[i] << std::endl;
        // if (expl_str.size() > 1 && expl_str[0] == path) {
		if (expl[i].size() > 1) {
            pid = stoi(expl[i]);
            break;
        }
    }

    std::cout << "find_pid_by_path: " << pid << std::endl;

    return pid;
}

unsigned int count_proc_in_path(const char *path)
{
    std::string spath = std::string(path);

	boost::replace_all(spath, "\\", "\\\\");
    std::string result = exec(str(boost::format("wmic process where \"executablepath like '%s%%'\" get processid") % spath));
	std::cout << "result: " << result << std::endl;

    std::vector<std::string> expl = explode("\n", result);
    unsigned int pcount = 0;
    for (int i = 1; i < expl.size(); i++) {
		if (expl[i].size() > 1) {
            pcount++;
        }
    }
    
    std::cout << "count_proc_in_path: " << pid << std::endl;

    return pcount;
}

void killall(const char *path)
{
    std::string spath = std::string(path);

	boost::replace_all(spath, "\\", "\\\\");
    std::string result = exec(str(boost::format("wmic process where \"executablepath like '%s%%'\" get processid") % spath));
	std::cout << "result: " << result << std::endl;

    std::vector<std::string> expl = explode("\n", result);
    for (int i = 1; i < expl.size(); i++) {
		if (expl[i].size() > 1) {
            std::string cmd_kill_str = "taskkill /F /PID " + expl[i];
            exec(&cmd_kill_str[0]);
        }
    }
}

#endif
