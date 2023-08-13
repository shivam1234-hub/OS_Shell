#include "redirection.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;


void Redirection::setupRedirection(vector<string>& args, int& stdin_fd, int& stdout_fd) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "<") {
            if (i + 1 < args.size()) {
                stdin_fd = open(args[i + 1].c_str(), O_RDONLY);
                if (stdin_fd == -1) {
                    std::cerr << "Error opening file for input redirection." << std::endl;
                    exit(EXIT_FAILURE);
                }
                args.erase(args.begin() + i, args.begin() + i + 2);
                break;
            }
        } else if (args[i] == ">") {
            if (i + 1 < args.size()) {
                stdout_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (stdout_fd == -1) {
                    std::cerr << "Error opening file for output redirection." << std::endl;
                    exit(EXIT_FAILURE);
                }
                args.erase(args.begin() + i, args.begin() + i + 2);
            }
        } else if (args[i] == ">>") {
            if (i + 1 < args.size()) {
                stdout_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (stdout_fd == -1) {
                    std::cerr << "Error opening file for output redirection." << std::endl;
                    exit(EXIT_FAILURE);
                }
                args.erase(args.begin() + i, args.begin() + i + 2);
            }
        }
    }
}
