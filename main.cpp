#include <string>
#include <vector>

#include <iostream>

#include <cstdio>
#include <sys/types.h>

#ifdef __linux__
    #include <thread>
    #include <unistd.h>
    #include <csignal>

    #include <pwd.h>
    #include <sys/stat.h>
#elif _WIN32
	#include <process.h>

	#include <windows.h>
	#include <tlhelp32.h>
	#include <tchar.h>

	#if DEBUG
		#pragma comment(lib, "libboost_system-vc100-mt-gd-1_60.lib")
		#pragma comment(lib, "libboost_filesystem-vc100-mt-gd-1_60.lib")
		#pragma comment(lib, "libboost_iostreams-vc100-mt-gd-1_60.lib")
	#else
		#pragma comment(lib, "libboost_system-vc100-mt-1_60.lib")
		#pragma comment(lib, "libboost_filesystem-vc100-mt-1_60.lib")
		#pragma comment(lib, "libboost_iostreams-vc100-mt-1_60.lib")
	#endif

#endif

#include <cstdlib>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#include "proc.h"

#define GAS_PID_FILE "pid.txt"
#define GAS_INPUT_FILE "input.txt"
#define GAS_OUTPUT_FILE "output.txt"

#if defined(BOOST_POSIX_API)
    #define PROC_SHELL "sh"
    #define SHELL_PREF "-c"
#elif defined(BOOST_WINDOWS_API)
    #define PROC_SHELL "cmd"
    #define SHELL_PREF "/c"
#endif

using namespace boost::iostreams;

namespace bp = boost::process;
namespace bi = boost::iostreams;

std::string cmd_type    = "";
std::string command     = "";
std::string directory   = "";
std::string user        = "";
std::string upassword   = "";

bool no_stdin   = false;
bool no_stdout  = false;

// ---------------------------------------------------------------------

void show_help()
{
    std::cout << "**************************************\n";
    std::cout << "*     Welcome to GameAP Starter      *\n";
    std::cout << "**************************************\n\n";

    std::cout << "Program created by kNiK \n";
    std::cout << "GitHub: https://github.com/et-nik \n";
    std::cout << "Site: http://www.gameap.ru \n";

    std::cout << "Version: 0.2\n";
    std::cout << "Build date: " << __DATE__ << " " << __TIME__ << std::endl << std::endl;

    std::cout << "Parameters\n";
    std::cout << "-t <cmd_type>\n";
    std::cout << "-d <work dir>\n";
    std::cout << "-c <command>  (example 'hlds_run -game valve +ip 127.0.0.1 +port 27015 +map crossfire')\n\n";

    std::cout << "Examples:\n";
    std::cout << "./starter -t start -d /home/servers/hlds -c \"hlds_run -game valve +ip 127.0.0.1 +port 27015 +map crossfire\"\n";
}

// ---------------------------------------------------------------------

void run()
{
	boost::filesystem::current_path(&directory[0]);

    size_t arg_start = command.find(' ');
    std::string exe = command.substr(0, arg_start);

	std::string args;
	if (arg_start != std::string::npos) {
		args = command.substr(arg_start, command.size());
	}

    // Create and trunc stdout.txt
    {
        std::ofstream conout;
        conout.open(GAS_OUTPUT_FILE, std::ofstream::out | std::ofstream::trunc);
        conout.close();
    }

    unsigned long pid;

    boost::asio::io_service ios;
    bp::async_pipe pipe_in(ios);
    bp::opstream in;

    bp::child c(bp::search_path(PROC_SHELL),
                bp::args={SHELL_PREF, command},
                (bp::std_out & bp::std_err) > GAS_OUTPUT_FILE,
                bp::std_in < pipe_in,
                ios,
                bp::on_exit([&](int exit, const std::error_code& ec_in)
                {
                    pipe_in.async_close();
                })
    );

    pid = static_cast<unsigned long>(c.id());

    std::string input_line;

    // Create and trunc stdin.txt
    std::fstream coninput;
    std::ofstream conout;
    conout.open(GAS_INPUT_FILE, std::ofstream::out | std::ofstream::trunc);
    conout.close();

	// Cmd
    std::ofstream cmdlog;
    cmdlog.open("cmd.txt", std::ofstream::out | std::ofstream::trunc);
	cmdlog << args << std::endl;
    cmdlog.close();

    // Write pid
    std::ofstream pidfile;
    pidfile.open(GAS_PID_FILE, std::ofstream::out | std::ofstream::trunc);
    pidfile << pid;
    pidfile.close();

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    if (!no_stdin) {
        while(true) {
            if (!c.running()) {
                break;
            }

            coninput.open(GAS_INPUT_FILE, std::ifstream::in);
            getline(coninput, input_line);

            if (!input_line.empty()) {
                if (input_line == "GAS_EXIT") {
                    coninput.close();
                    c.terminate();
                    break;
                }

                boost::asio::async_write(pipe_in, boost::asio::buffer(input_line + "\n\r"), []( const boost::system::error_code&, std::size_t){});

                conout.open(GAS_INPUT_FILE, std::ofstream::out | std::ofstream::trunc);
                conout.close();
            }

            coninput.close();

            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    ios.run();
}

#ifdef _WIN32
bool isRunning(int pid)
{
   HANDLE pss = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

   PROCESSENTRY32 pe = { 0 };
   pe.dwSize = sizeof(pe);

	if (Process32First(pss, &pe)) {
		do {
			// pe.szExeFile can also be useful
			if (pe.th32ProcessID == pid)
				return true;
		}
		while(Process32Next(pss, &pe));
	}

	CloseHandle(pss);

	return false;
}
#endif

// ---------------------------------------------------------------------

bool server_status()
{
    char bufread[32];

    pid_t pid = 0;

    boost::filesystem::current_path(&directory[0]);
    boost::filesystem::path p(&directory[0]);
    p /= "pid.txt";

    std::ifstream pidfile;
    pidfile.open(p.string(), std::ios::in);

    if (pidfile.good()) {
        pidfile.getline(bufread, 32);

        pid = atoi(bufread);
    } else {
        std::cerr << "Pidfile " << p << " read error" << std::endl;
        unsigned int pcount = count_proc_in_path(&directory[0]);
        std::cout << "pcount: " << pcount << std::endl;

        if (pcount == 1) {
            pid = find_pid_by_path(&directory[0]);

            if (pid == -1) {
                std::cerr << "Pid not found" << std::endl;
                return false;
            }
        }
    }

    pidfile.close();

    bool active = false;

    if (pid != 0) {
        #ifdef __linux__
            active = kill(pid, 0) == 0;
        #elif _WIN32
			active = isRunning(pid);
        #endif
    }

	if (!active) {
		unsigned int pcount = count_proc_in_path(&directory[0]);

		if (pcount > 0) {
            pid_t fpid = find_pid_by_path(&directory[0]);
            
            if (fpid != pid && fpid > 1) {
                // Write new pid
                std::ofstream pidfile;
                pidfile.open(GAS_PID_FILE, std::ofstream::out | std::ofstream::trunc);
                pidfile << fpid;
                pidfile.close();

                active = true;
            }
		}
	}

    return active;
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc - 1; i++) {
        if (std::string(argv[i]) == "-t") {
            cmd_type = argv[i + 1];
            i++;
        }
        else if (std::string(argv[i]) == "-d") {
            directory = argv[i + 1];
            i++;
        }
        else if (std::string(argv[i]) == "-u") {
            user = argv[i + 1];
            i++;
        }
        else if (std::string(argv[i]) == "-p") {
            upassword = argv[i + 1];
            i++;
        }
        else if (std::string(argv[i]) == "--disable-stdin") {
            no_stdin = true;
            i++;
        }
        else if (std::string(argv[i]) == "--disable-stdout") {
            no_stdout = true;
            i++;
        }
        else if (std::string(argv[i]) == "-c" || std::string(argv[i]) == "--cmd") {
            while (i < argc - 1) {
				std::string argv_str = std::string(argv[i + 1]);

				if (argv_str.find(" ") != std::string::npos) {
					argv_str = '"' + argv_str + '"';
				}

                command = (command.empty())
                        ? command + argv_str
                        : command + std::string(" ") + argv_str;
                i++;
            }

            // Cut quotes
            if (!command.empty() && command[0] == '"' && command[command.length()-1] == '"') {
                command = command.substr(1, command.length()-2);
            }

            i++;
        }
        else {
            // ...
        }
    }

	std::cout << "Command: " << command << std::endl;
	std::cout << "Type: " << cmd_type << std::endl;

    if (cmd_type == "start") {
        #ifdef __linux__
            passwd * pwd;
            if (!user.empty()) {
                pwd = getpwnam(&user[0]);

                if (pwd == nullptr) {
                    std::cerr << "Invalid user" << std::endl;
                    return 1;
                }
            }

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            int i;
            for (i = getdtablesize(); i >= 0; --i) close(i);

            pid_t pid = fork();

            if (pid == -1) {
                std::cerr << "Fork error" << std::endl;
                return 1;
            }

            if (pid != 0) {
                exit(0);
            }

            if (pid == 0) {
                umask(0);
                setsid();

                signal(SIGHUP, SIG_IGN);
                setpgrp();

                std::cout << "Child PID: " << getpid() << std::endl;
                std::cout << "Child Group: " << getppid() << std::endl;

                if (user != "") {
                    setgid(pwd->pw_gid);
                    setuid(pwd->pw_uid);
                }

                freopen("/dev/null", "r", stdin);
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);

                run();
            }
        #elif _WIN32
            std::string cmd = "/c " + std::string(argv[0]) + " -t run -d " + directory + " -c " + command;
            // std::string cmd = std::string(argv[0]) + " -t run -d " + directory + " -c " + command;

            TCHAR* szCmdline = new TCHAR[strlen(cmd.c_str()) + 1];
            mbstowcs(szCmdline, cmd.c_str(), strlen(cmd.c_str()) + 1);

            TCHAR* szUser = new TCHAR[strlen(user.c_str()) + 1];
            mbstowcs(szUser, user.c_str(), strlen(user.c_str()) + 1);

            TCHAR* szPassword = new TCHAR[strlen(upassword.c_str()) + 1];
            mbstowcs(szPassword, upassword.c_str(), strlen(upassword.c_str()) + 1);


            HANDLE hUserToken;
			TOKEN_PRIVILEGES tp;
            PROCESS_INFORMATION pi;
            STARTUPINFO         si;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

			// si.lpDesktop = "";
            si.lpDesktop = L"winsta0\\default";
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;

            if (user != "" && upassword != "") {

                if (!LogonUser(
                    szUser,
                    NULL,
                    szPassword,
                    LOGON32_LOGON_INTERACTIVE,
                    LOGON32_PROVIDER_DEFAULT,
                    &hUserToken
                )) {
                    std::cerr << "LogonUser error: " << GetLastError() << std::endl;
                    return 1;
                }

                if (!CreateProcessAsUser(
                    hUserToken,
					bp::shell_path().wstring().c_str(),
                    szCmdline,
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_NEW_CONSOLE,
                    NULL,
                    NULL,
                    &si,
                    &pi
                )) {
                    std::cerr << "CreateProcessAsUser error: " << GetLastError() << std::endl;
                    return 1;
                }
            }
            else {
                // std::string shell_path = bp::shell_path().string();

                if (!CreateProcess(
                    bp::shell_path().wstring().c_str(),
                    szCmdline,
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_NEW_CONSOLE,
                    NULL,
                    NULL,
                    &si,
                    &pi
                )) {
                    std::cerr << "CreateProcess error: " << GetLastError() << std::endl;
                    return 1;
                }
            }

            CloseHandle(hUserToken);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            delete szCmdline;
            delete szUser;
            delete szPassword;

            fclose(stdin);
            fclose(stdout);
            fclose(stderr);
        #endif
    }
    else if (cmd_type == "stop") {
        boost::filesystem::current_path(&directory[0]);

        int pid;

        std::fstream pidfile;
        pidfile.open(GAS_PID_FILE, std::ifstream::in);

        if (!pidfile.good()) {
            std::cerr << "PID file open error" << std::endl;
            unsigned int pcount = count_proc_in_path(&directory[0]);
            std::cout << "pcount: " << pcount << std::endl;

            if (pcount == 1) {
                pid = find_pid_by_path(&directory[0]);

                if (pid == -1) {
                    std::cerr << "Pid not found" << std::endl;
                    return 0;
                }
            } else if (pcount > 1) {
                killall(&directory[0]);
            }
        } else {
            std::string stpid;
            getline(pidfile, stpid);

            pid = atoi(&stpid[0]);
        }

        pidfile.close();

        if (!server_status()) {
            pid = find_pid_by_path(&directory[0]);

            // Kill all in dir. AHAHAHAHAAA!!!
            if (pid > 0) {
                killall(&directory[0]);
            }
        } else {

            std::cout << "PID: " << pid << std::endl;

            if (pid != -1) {
                #ifdef __linux__
                    if (kill(pid, SIGTERM) != 0) {
                        std::cerr << "Stop error: " << strerror(errno) << std::endl;
                    }
                #elif _WIN32
					std::string stpid;
					//itoa(pid, &stpid[0], 10);
					//stpid = std::string(itoa(pid));

					std::string cmd_kill_str = "taskkill /F /PID " + std::to_string((_Longlong)pid);
					std::cout << "stpid: " << stpid << std::endl;
					std::cout << "CMD KILL: " << cmd_kill_str << std::endl;
                    system(&cmd_kill_str[0]);
                #endif
            } else {
                std::cerr << "Server not running" << std::endl;
                return 3;
            }
        }

        // boost::filesystem::remove(GAS_PID_FILE);
    }
#ifdef _WIN32
    else if (cmd_type == "run") {
        try {
            run();
        } catch (std::exception &e) {
            std::cerr << "Run error: " << e.what() << std::endl;
			exit(1);
        }
    }
#endif
    else if (cmd_type == "status") {
        bool active = server_status();

        if (active) {
            std::cout << "Server still active" << std::endl;
            return 0;
        }
        else {
            std::cout << "Server not running" << std::endl;
            return 3;
        }
    }
    else {
        show_help();
    }

    return 0;
}
