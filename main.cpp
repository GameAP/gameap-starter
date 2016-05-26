#include <string> 
#include <vector>

#include <thread>
#include <iostream>

#include <stdio.h>
#include <sys/types.h>

#ifdef __linux__ 
    #include <unistd.h>
    #include <signal.h>
#endif

#include <stdlib.h>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#define GAS_PID_FILE "pid.txt"
#define GAS_INPUT_FILE "stdin.txt"
#define GAS_OUTPUT_FILE "stdout.txt"

#if defined(BOOST_POSIX_API)
    #define PROC_SHELL "sh"
    #define SHELL_PREF "-c"
#elif defined(BOOST_WINDOWS_API)
    #define PROC_SHELL "cmd"
    #define SHELL_PREF "/c"
#endif 

using namespace boost::process;
using namespace boost::process::initializers;
using namespace boost::iostreams;

std::string type        = "";
std::string command     = "";
std::string directory   = "";
std::string user        = "";

bool no_stdin   = false;
bool no_stdout  = false;


void show_help()
{
    std::cout << "**************************************\n";
    std::cout << "*     Welcome to GameAP Starter      *\n";
    std::cout << "**************************************\n\n";

    std::cout << "Program created by NiK \n";
    std::cout << "Site: http://www.gameap.ru \n";
    std::cout << "Skype: kuznets007 \n\n";
    
    std::cout << "Version: 0.1\n";
    std::cout << "Build date: " << __DATE__ << " " << __TIME__ << std::endl << std::endl;

    std::cout << "Parameters\n";
    std::cout << "-t <type>\n";
    std::cout << "-d <work dir>\n";
    std::cout << "-c <command>  (example 'hlds_run -game valve +ip 127.0.0.1 +port 27015 +map crossfire')\n\n";

    std::cout << "Examples:\n";
    std::cout << "./starter -t start -d /home/servers/hlds -c \"hlds_run -game valve +ip 127.0.0.1 +port 27015 +map crossfire\"\n";
}

void run()
{
    boost::filesystem::current_path(&directory[0]);
    
    file_descriptor_sink sink(GAS_OUTPUT_FILE);

    boost::process::pipe pin = boost::process::create_pipe();
    boost::iostreams::file_descriptor_source source(pin.source, boost::iostreams::close_handle);

    size_t arg_start = command.find(' ');
    std::string exe = command.substr(0, arg_start);
    std::string args = command.substr(arg_start, command.size());

    unsigned long pid;

    #ifdef __linux__
        chdir(&directory[0]);
        child c = boost::process::execute(
            bind_stdout(sink),
            bind_stderr(sink),
            bind_stdin(source),
            start_in_dir(directory),
            boost::process::initializers::run_exe(shell_path()),
            boost::process::initializers::set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, command}),

            boost::process::initializers::inherit_env(),
            boost::process::initializers::throw_on_error()
        );
        pid = c.pid;
        
    #elif _WIN32
        args = directory + "\\" + exe + " " + args;
        wchar_t* szArgs = new wchar_t[strlen(args.c_str()) + 1];
        mbstowcs(szArgs, args.c_str(), strlen(args.c_str()) + 1);

        child c = boost::process::execute(
            bind_stdout(sink),
            bind_stderr(sink),
            bind_stdin(source),
            run_exe(exe),
            boost::process::initializers::set_cmd_line(szArgs),
            start_in_dir(directory),
            boost::process::initializers::inherit_env(),
            boost::process::initializers::throw_on_error(),
            show_window(SW_HIDE)
        );
        pid = c.proc_info.dwProcessId;
    #endif

    file_descriptor_sink sink2(pin.sink, boost::iostreams::close_handle);
    boost::iostreams::stream<boost::iostreams::file_descriptor_sink> os(sink2);

    std::string input_line;

    // Create and trunc stdin.txt
    std::fstream coninput;
    std::ofstream conout;
    conout.open(GAS_INPUT_FILE, std::ofstream::out | std::ofstream::trunc);
    conout.close();

    // Write pid
    std::ofstream pidfile;
    pidfile.open(GAS_PID_FILE, std::ofstream::out | std::ofstream::trunc);
    pidfile << pid;
    pidfile.close();

    if (no_stdin) {
        wait_for_exit(c);
    } else {
        #ifdef __linux__
            bool exited = false;
            std::thread thr{[c](bool *exited) {
                boost::system::error_code ec;
                wait_for_exit(c, ec);
                *exited = true;
            }, &exited};
        #elif _WIN32
            unsigned long exit = 0;
        #endif
    
        while(true) {
            #ifdef __linux__
                if (exited) {
                    kill(0, SIGINT);
                }
            #elif _WIN32
                GetExitCodeProcess(c.proc_info.hProcess, &exit);

                if (exit != STILL_ACTIVE && exit != 4294967295) {
                    break;
                }
            #endif
            
            coninput.open(GAS_INPUT_FILE, std::ifstream::in);
            getline(coninput, input_line);
            
            if (input_line != "") {
                if (input_line == "GAS_EXIT") {
                    coninput.close();

                    #ifdef __linux__ 
                        kill(0, SIGINT);
                    #elif _WIN32
                        terminate(c);
                    #endif
                    
                    break;
                }
                
                os << input_line << std::endl;

                conout.open(GAS_INPUT_FILE, std::ofstream::out | std::ofstream::trunc);
                conout.close();
            }
            else {
                // std::cout << "input line empty " << std::endl;
            }

            coninput.close();

            #ifdef __linux__ 
                sleep(1);
            #elif _WIN32
                Sleep(1000);
            #endif
        }
    }
    
    // wait_for_exit(c);
}

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc - 1; i++) {
        if (std::string(argv[i]) == "-t") {
            type = argv[i + 1];
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
        else if (std::string(argv[i]) == "--disable-stdin") {
            no_stdin = true;
            i++;
        }
        else if (std::string(argv[i]) == "--disable-stdout") {
            no_stdout = true;
            i++;
        }
        else if (std::string(argv[i]) == "-c") {
            while (i < argc - 1) {
                command = (command == "") ? command + argv[i + 1] : command + " " + argv[i + 1];
                i++;
            }

            i++;
        }
        else {
            // ...
        }
    }
    
    if (type == "start") {

        #ifdef __linux__
            pid_t pid = fork();

            if (pid == -1) {
                std::cerr << "Fork error" << std::endl;
                return 1;
            }

            if (pid == 0) {
                run();
            } else {
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
            }
        #elif _WIN32
            
            try {
                std::string cmd = "/c " + std::string(argv[0]) + " -t run -d " + directory + " -c " + command;

                wchar_t* szCmdline = new wchar_t[strlen(cmd.c_str()) + 1];
                mbstowcs(szCmdline, cmd.c_str(), strlen(cmd.c_str()) + 1);

                boost::process::execute(
                    boost::process::initializers::close_stdout(),
                    boost::process::initializers::close_stderr(),
                    boost::process::initializers::close_stdin(),
                    boost::process::initializers::run_exe(shell_path()),
                    boost::process::initializers::set_cmd_line(szCmdline),
                    boost::process::initializers::inherit_env(),
                    boost::process::initializers::throw_on_error(),
                    show_window(SW_HIDE)
                );
            } catch (boost::system::system_error &e) {
                std::cerr << "Exec error: " << e.what() << std::endl;
            }
            
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        #endif
    }
    else if (type == "stop") {
        boost::filesystem::current_path(&directory[0]);

        std::fstream pidfile;
        pidfile.open(GAS_PID_FILE, std::ifstream::in);

        if (!pidfile.good()) {
            std::cerr << "PID file open error" << std::endl;
            return 1;
        }

        std::string stpid;
        getline(pidfile, stpid);

        pid_t pid = atoi(&stpid[0]);

        #ifdef __linux__
            if (kill(pid, SIGTERM) != 0) {
                std::cout << "Stop error" << std::endl;
            }
        #elif _WIN32
            std::string cmd_kill_str = "taskkill /PID " + jroot["pid"].asString();
			wchar_t* cmd_kill = new wchar_t[strlen(cmd_kill_str.c_str()) + 1];
			mbstowcs(cmd_kill, cmd_kill_str.c_str(), strlen(cmd_kill_str.c_str()) + 1);

			::_tsystem(cmd_kill);
        #endif

        pidfile.close();
        boost::filesystem::remove(GAS_PID_FILE);
    }
#ifdef _WIN32
    else if (type == "run") {
        try {
            run();
        } catch (boost::system::system_error &e) {
            std::cerr << "Run error: " << e.what() << std::endl;
        }
    }
#endif
    else {
        show_help();
    }

    return 0;
}
