#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "redirection.h"
#include "pipe.h"
#include <thread>
#include <pthread.h>

using namespace std;
#define veci vector<int> 
#define vecs vector<string>
#define pb push_back
#define all(x) (x).begin(), (x).end() 
#define FOR(s,n) for(int i = s; i < n; i++)
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define UNDERLINE "\033[4m"
#define CLEAR   "\033[2J\033[1;1H"


template<typename T> 
ostream& operator<<(ostream& COUT, vector<T>& v){ for(int i=0 ; i<v.size() ; i++){ COUT<<v[i]<<" "; } COUT << endl; return COUT; }
template<typename T> 
istream& operator>>(istream& CIN, vector<T>& a){ for(int i=0 ; i<a.size() ; i++) CIN>>a[i]; return CIN; }
template<typename T> 
void pws(const T& arg){ cout << arg <<endl;}
template <typename T, typename... Args>
void pws(const T& first, const Args&... args) {cout << first << " ";pws(args...);}



void clearScreen() {
    std::cout << CLEAR;
}

void printWelcomeMessage() {
    std::cout << BOLD << MAGENTA; // Bold and blue text
    std::cout << "**************************************" << std::endl;
    std::cout << "*         Welcome to My Shell        *" << std::endl;
    std::cout << "*                                    *" << std::endl;
    std::cout << "*          Let's get Started         *" << std::endl;
    std::cout << "**************************************" << RESET << std::endl;
}

void init() {
    clearScreen();
    printWelcomeMessage();
    usleep(1000000);
    clearScreen();
}

set<string> commandHistory;

void showCommandHistory(int count) {
    int numCommandsToShow = min(count, static_cast<int>(commandHistory.size()));
    auto it = commandHistory.rbegin();
    for (int i = 0; i < numCommandsToShow; ++i) {
        cout << "[" << i + 1 << "] " << *it << endl;
        ++it;
    }
}

pthread_t bgCheckerThread;

struct BackgroundProcess {
    pid_t pid;
    string command;
    bool isRunning;
};




vector<BackgroundProcess> backgroundProcesses;

void checkBackgroundProcesses() {
    // Iterate through backgroundProcesses and check for terminated processes
    for (size_t i = 0; i < backgroundProcesses.size(); ++i) {
        BackgroundProcess& bgProcess = backgroundProcesses[i];

        // Use waitpid with WNOHANG to check if the process has terminated
        int status;
        pid_t result = waitpid(bgProcess.pid, &status, WNOHANG);

        if (result == -1) {
            // Error occurred, handle it
        } else if (result == 0) {
            // Process is still running
        } else {
            // Process has terminated
            bgProcess.isRunning = false;
            cout << "[" << bgProcess.pid << "] Background process finished: " << bgProcess.command << endl;
            cout << BOLD << BLUE << "Shell>> " << RESET;
            cout.flush();
            backgroundProcesses.erase(backgroundProcesses.begin() + i);
            --i; // Decrement i to account for the removed element
        }
    }
}

void* backgroundProcessChecker(void* arg) {
    while (true) {
        checkBackgroundProcesses();
        usleep(1000000); // Wait for a short interval before checking again
    }
    return nullptr;
}

void executeCommand(string& command,int runInBackground){

    if (command.find('|') != string::npos) { // Check if the command has a pipe
        PipeHandler::executePipedCommands(command); // Use the pipe handler for piped commands
        return;
    } 

    pid_t pid = fork();

    if(pid==0){
         istringstream iss(command);
         vecs args;
         string arg;

         while(iss>>arg){
            args.push_back(arg);
         }

        int stdin_fd = 0;
        int stdout_fd = 1;

        Redirection::setupRedirection(args, stdin_fd, stdout_fd);

        dup2(stdin_fd, 0);
        dup2(stdout_fd, 1);

        vector<char*> cArgs; // in C-style string 

         for(string& arg:args){
            cArgs.push_back(const_cast<char*>(arg.c_str()));
         }

         cArgs.pb(nullptr);

         execvp(cArgs[0],cArgs.data());
         pws("Could Not Execute command");// If execvp return means it gets failed
         exit(0);

    }
    else if(pid>0){
        if(!runInBackground){
            waitpid(pid, nullptr, 0);
        } else{
              // Add the process to the backgroundProcesses list
            BackgroundProcess bgProcess;
            bgProcess.pid = pid;
            bgProcess.command = command;
            bgProcess.isRunning = true;
            backgroundProcesses.push_back(bgProcess);

            // Print background process information
            cout << "[" << pid << "] Background process started: " << command << endl;

        }
    } else {
          
          pws("Forking Failed");
    }

}

int promptShow = 0;


void sigintHandler(int signum) {
    cout<<"\b"<<"C"<<"\n"; // Clear line and print prompt
    if(promptShow) cout << BOLD << BLUE << "Shell>> " << RESET;
    cout.flush();
}

void terminateShell() {
    // Cleanup and terminate the shell
    pthread_cancel(bgCheckerThread);
    pthread_join(bgCheckerThread, nullptr);
    exit(0);
}



signed main(){
    init();
    string input;

    // Start the background process checker thread
    // thread bgCheckerThread(backgroundProcessChecker);
    pthread_create(&bgCheckerThread, nullptr, backgroundProcessChecker, nullptr);
    signal(SIGINT,sigintHandler);

    while(1){
        promptShow = 1;
        cout << BOLD << BLUE << "Shell>> " << RESET;
        getline(cin,input);
        promptShow = 0;
 
        if(input=="exit") {
            cout<<"Bye!"<<endl;
            terminateShell();
        }

        if(input=="history"){
            showCommandHistory(1000);
            continue;
        }

        commandHistory.insert(input);

            // Limit the history size to 10000 commands
            if (commandHistory.size() > 10000) {
                auto it = commandHistory.begin();
                advance(it, commandHistory.size() - 10000);
                commandHistory.erase(commandHistory.begin(), it);
        }


        // Check if the command should run in the background
        bool runInBackground = false;
        if (!input.empty() && input.back() == '&') {
            input.pop_back();
            runInBackground = true;
        }

        executeCommand(input,runInBackground);     
    }  

    terminateShell();
}
