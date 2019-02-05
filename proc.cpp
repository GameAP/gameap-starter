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

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// ---------------------------------------------------------------------

std::vector<pid_t> find_pids(const char *path)
{
    pid_t starter_pid = getpid();
    pid_t pid = -1;

    std::vector<pid_t> result;

    for(auto& p: fs::directory_iterator("/proc")) {
        if (fs::is_directory(p)) {

            fs::path exe = p / "exe";
            fs::path cwd = p / "cwd";

            int cpid = atoi(p.path().filename().c_str());

            if (cpid <= 0) {
                continue;
            }

            if (cpid == starter_pid) {
                continue;
            }

            try {

                if (fs::exists(exe) && fs::is_symlink(exe)) {
                    exe = fs::read_symlink(exe);
                    cwd = fs::read_symlink(cwd);

                    // Ignore shell
                    if (exe.filename() == "bash"
                        || exe.filename() == "rbash"
                        || exe.filename() == "dash"
                        || exe.filename() == "sh"
                            ) {
                        continue;
                    }

                    std::string filename = exe.filename().string();

                    if (strcmp(exe.parent_path().c_str(), path) == 0) {
                        result.insert(result.end(), cpid);
                    } else if (strcmp(cwd.c_str(), path) == 0) {
                        result.insert(result.end(), cpid);
                    }
                }
            } catch (fs::filesystem_error &e) {
                continue;
            }

        }
    }

    return result;
}

// ---------------------------------------------------------------------

pid_t find_pid_by_path(const char *path)
{
    return find_pids(path).front();
}

// ---------------------------------------------------------------------

unsigned int count_proc_in_path(const char *path)
{
    return static_cast<unsigned int>(find_pids(path).size());
}

// ---------------------------------------------------------------------

void killall(const char *path)
{
    std::vector<pid_t> pids = find_pids(path);

    for (const pid_t& pid : find_pids(path)) {
        std::cout << "Killing: " << pid << std::endl;

        if (kill(pid, SIGTERM) != 0) {
            std::cerr << "Stop error (killall): " << strerror(errno) << std::endl;
        }
    }
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
