#include <string> 
#include <vector> 
#include <iostream>

#include <stdio.h>
#include <sys/types.h>

#ifdef __linux__ 
	#include <unistd.h>
#endif

#include <stdlib.h>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

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

void run(std::string command, std::string directory)
{
    std::cout << "RUN" << std::endl;
    std::cout << "directory: " << directory << std::endl;
    std::cout << "command: " << command << std::endl;

	boost::filesystem::current_path(&directory[0]);
    
    file_descriptor_sink sink(GAS_OUTPUT_FILE);

    boost::process::pipe pin = boost::process::create_pipe();
    boost::iostreams::file_descriptor_source source(pin.source, boost::iostreams::close_handle);

	#ifdef __linux__ 
		child c = boost::process::execute(
			bind_stdout(sink),
			bind_stderr(sink),
			bind_stdin(source),
			boost::process::initializers::run_exe(shell_path()),
			start_in_dir(directory),
			boost::process::initializers::set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, command}),
			boost::process::initializers::inherit_env(),
			boost::process::initializers::throw_on_error()
		);
	#elif _WIN32
		command = SHELL_PREF + std::string(" ") + command;
		wchar_t* szCmdline = new wchar_t[strlen(command.c_str()) + 1];
		mbstowcs(szCmdline, command.c_str(), strlen(command.c_str()) + 1);

		child c = boost::process::execute(
			bind_stdout(sink),
			bind_stderr(sink),
			bind_stdin(source),
			run_exe(shell_path()),
			// set_cmd_line(command), // MinGW
			set_cmd_line(szCmdline),
			start_in_dir(directory),
			boost::process::initializers::inherit_env(),
			boost::process::initializers::throw_on_error(),
			show_window(SW_HIDE)
		);
	#endif

    file_descriptor_sink sink2(pin.sink, boost::iostreams::close_handle);
    boost::iostreams::stream<boost::iostreams::file_descriptor_sink> os(sink2);

    std::string input_line;

    while(true) {
        std::fstream coninput;
        coninput.open(GAS_INPUT_FILE, std::ifstream::in);
        getline(coninput, input_line);
        
        if (input_line != "") {
            if (input_line == "GAS_EXIT") {
				coninput.close();
				terminate(c);
                break;
            }
            
            os << input_line << std::endl;

            std::ofstream conout;
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
    };

    wait_for_exit(c);
}

int main(int argc, char *argv[])
{
    std::string type		= "";
	std::string command		= "";
	std::string directory	= "";

    for (int i = 0; i < argc - 1; i++) {
		if (std::string(argv[i]) == "-t") {
			type = argv[i + 1];
			i++;
		}
		else if (std::string(argv[i]) == "-d") {
			directory = argv[i + 1];
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
    
    std::cout << "exe: " << argv[0] << std::endl;
    std::cout << "type: " << type << std::endl;
    std::cout << "directory: " << directory << std::endl;
    std::cout << "command: " << command << std::endl;

    if (type == "start") {

		#ifdef __linux__ 
			boost::process::execute(
				boost::process::initializers::run_exe(shell_path()),
				boost::process::initializers::set_args(std::vector<std::string>{PROC_SHELL, SHELL_PREF, std::string(argv[0]) + " -t run -d " + directory + " -c " + command}),
				boost::process::initializers::inherit_env(),
				boost::process::initializers::throw_on_error()
			);
		#elif _WIN32
			try {
				std::string cmd = "/c " + std::string(argv[0]) + " -t run -d " + directory + " -c " + command;

				wchar_t* szCmdline = new wchar_t[strlen(cmd.c_str()) + 1];
				mbstowcs(szCmdline, cmd.c_str(), strlen(cmd.c_str()) + 1);

				boost::process::execute(
					boost::process::initializers::run_exe(shell_path()),
					boost::process::initializers::set_cmd_line(szCmdline),
					// boost::process::initializers::set_cmd_line(cmd), // MinGW
					boost::process::initializers::inherit_env(),
					boost::process::initializers::throw_on_error()
				);
			} catch (boost::system::system_error &e) {
				std::cerr << "Exec error: " << e.what() << std::endl;
			}
		#endif
    }
    else if (type == "run") {
        run(command, directory);
    }

    return 0;
}
