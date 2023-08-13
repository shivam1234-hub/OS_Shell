#include "pipe.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

void PipeHandler::executePipedCommands(string& command) {
    vector<string> commands;
    size_t start = 0;
    size_t end;

    while ((end = command.find('|', start)) != string::npos) {
        commands.push_back(command.substr(start, end - start));
        start = end + 1;
    }
    commands.push_back(command.substr(start)); // Last command

    int noPipes = commands.size()-1;

   int pipes[noPipes][2];

    for(int i=0;i<noPipes;i++) pipe(pipes[i]);

    pid_t pid;
    size_t cmdIndex = 0;

    for( string& cmd:commands){
        pid = fork();
        if (pid == 0) { // Child process
            if (cmdIndex != 0) {
                dup2(pipes[cmdIndex - 1][0], 0);
            }
            if (cmdIndex != commands.size() - 1) {
                dup2(pipes[cmdIndex][1], 1);
            }

            for (size_t i = 0; i < noPipes; ++i) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            execlp("sh", "sh", "-c", cmd.c_str(), NULL);
            std::cerr << "Error executing command." << std::endl;
            exit(EXIT_FAILURE);
        } else if (pid > 0) { // Parent process
            // Close parent's copy of pipes
            if (cmdIndex != 0) {
                close(pipes[cmdIndex - 1][0]);
            }
            if (cmdIndex != commands.size() - 1) {
                close(pipes[cmdIndex][1]);
            }

            ++cmdIndex;
        } else {
            std::cerr << "Forking failed." << std::endl;
        }
    }

     // Close all pipes in parent after all children have been forked
    for (size_t i = 0; i < noPipes; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (size_t i = 0; i < commands.size(); ++i) {
        wait(NULL);
    }

}
