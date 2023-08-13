#include "signal.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t foregroundPid = -1;

void SignalHandler::handleCtrlC(int signal) {
    if (foregroundPid != -1) {
        kill(foregroundPid, SIGINT);
        foregroundPid = -1;
    }
    std::cout << std::endl; // Print a newline after interrupting the command
}

void SignalHandler::handleCtrlZ(int signal) {
    if (foregroundPid != -1) {
        kill(foregroundPid, SIGTSTP);
        foregroundPid = -1;
    }
    std::cout << std::endl; // Print a newline after stopping the command
}

void SignalHandler::setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = SignalHandler::handleCtrlC;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SignalHandler::handleCtrlZ;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
}
