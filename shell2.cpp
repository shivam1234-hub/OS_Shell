#include <iostream>
#include <algorithm>
#include <deque>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <stack>
#include <sys/inotify.h>
#include <sys/select.h>
#include <map>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "redirection.h"
#include "pipe.h"
#include "signal.h"
#include <thread>
#include <pthread.h>
#include <termios.h>

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

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

struct termios tio, oldtio;

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

struct state
{
    int len, link;
    map<char, int> next;
};

class SuffixAutomaton
{
    int text_size;
    vector<state> st;
    int sz, last;
    const string TEXT;
    void sa_init()
    {
        st[0].len = 0;
        st[0].link = -1;
        sz = 1;
        last = 0;
    }
    void sa_extend(char c)
    {
        int cur = sz++;
        st[cur].len = st[last].len + 1;
        int p = last;
        while (p != -1 && !st[p].next.count(c))
        {
            st[p].next[c] = cur;
            p = st[p].link;
        }
        if (p == -1)
        {
            st[cur].link = 0;
        }
        else
        {
            int q = st[p].next[c];
            if (st[p].len + 1 == st[q].len)
            {
                st[cur].link = q;
            }
            else
            {
                int clone = sz++;
                st[clone].len = st[p].len + 1;
                st[clone].next = st[q].next;
                st[clone].link = st[q].link;
                while (p != -1 && st[p].next[c] == q)
                {
                    st[p].next[c] = clone;
                    p = st[p].link;
                }
                st[q].link = st[cur].link = clone;
            }
        }
        last = cur;
    }

public:
    SuffixAutomaton(const string &text) : TEXT(text), text_size(text.size())
    {
        st.resize(2 * text_size + 10);
        sa_init();
        for (auto c : text)
            sa_extend(c);
    }

    int longest_prefix(string pattern)
    {
        int cur = 0;
        int len = 0;
        for (auto c : pattern)
        {
            if (st[cur].next.count(c))
            {
                cur = st[cur].next[c];
                len++;
            }
            else
            {
                return len;
            }
        }
        return len;
    }
    int longest_substring(string pattern)
    {
        int cur = 0;
        int len = 0;
        int temp = 0;
        string ans;

        for (int i = 0; i < pattern.size(); i++)
        {
            char c = pattern[i];
            if (st[cur].next.count(c))
            {
                temp++;
                cur = st[cur].next[c];
            }
            else
            {
                while (~cur and !st[cur].next.count(c))
                {
                    cur = st[cur].link;
                }
                if (~cur)
                {
                    temp = st[cur].len + 1;
                    cur = st[cur].next[c];
                }
                else
                {
                    cur = 0;
                    temp = 0;
                }
            }
            // i - temp + 1 is start of match
            if (len < temp)
            {
                ans = pattern.substr(i - temp + 1, temp);
                len = temp;
            }
        }
        // cout<<ans<<endl;
        return static_cast<int>(ans.size());
    }
    string returnFullString()
    {
        return TEXT;
    }
};

class history
{
private:
    history()
    {
        // read from file
        ifstream readstream(history_file_name, ios::in);
        if (!readstream.fail())
        {
            string s;
            while (getline(readstream, s))
                addHistory(s);
        }
        readstream.close();
    }

    static const int max_history;
    static const string history_file_name;
    // Command strings are stored in a deque, with the most recent ones being in the front
    deque<string> buff;
    deque<SuffixAutomaton> suffix_automata_history;

public:
    static history &shellHistoy()
    {
        static history sShellHistory;
        return sShellHistory;
    }

    void addHistory(string command)
    {
        buff.push_front(command);
        suffix_automata_history.push_front(SuffixAutomaton(command));
        if (buff.size() > max_history)
        {
            buff.pop_back();
            suffix_automata_history.pop_back();
        }
    }

    void saveHistory()
    {
        // write to file
        ofstream writestream(history_file_name, ios::out | ios::trunc);
        for (int i = getSize() - 1; i >= 0; i--)
            writestream << buff[i] << '\n';
        writestream.close();
    }

    const deque<string> &getHistory()
    {
        return buff;
    }

    int getSize()
    {
        return static_cast<int>(buff.size());
    }

    vector<string> return_matches(const string &s)
    {
        vector<string> ans;
        int slen = static_cast<int>(s.length());
        int len = 0;
        for (auto i : suffix_automata_history)
        {
            int lcslen = i.longest_substring(s);
            if (lcslen == slen)
            {
                ans.clear();
                ans.push_back(i.returnFullString());
                break;
            }
            if (lcslen > len && lcslen > 2)
            {
                ans.clear();
                ans.push_back(i.returnFullString());
                len = lcslen;
            }
            else if (lcslen == len && len != 0)
            {
                ans.push_back(i.returnFullString());
            }
        }
        return ans;
    }
};

const int history::max_history = 10000;
const string history::history_file_name = ".termhistory";



signed main(){

     init();
    // Start the background process checker thread
    // thread bgCheckerThread(backgroundProcessChecker);
    pthread_create(&bgCheckerThread, nullptr, backgroundProcessChecker, nullptr);
    signal(SIGINT,sigintHandler);

    tcgetattr(STDIN_FILENO, &tio);
    oldtio = tio;
    tio.c_lflag &= (~ICANON & ~ECHO);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tio);


    while(1){
        promptShow = 1;
        cout << BOLD << BLUE << "Shell>> " << RESET;
        int cnt = 0, history_nav = 1, history_cur = 0;
        char inp; 
        string input;

        do{
            inp = getchar();
            // for tab autocompletion
            if(inp == '\t'){

                history_nav = 0;
                long size;
                char *buf;
                string path;
                size = pathconf(".", _PC_PATH_MAX);
                if ((buf = (char *)malloc((size_t)size)) != NULL)
                    path = getcwd(buf, (size_t)size);

                string tab_inp;
                
                int cur = input.size()-1;
                while(cur>=0 and input[cur]!=' ')
                {
                     tab_inp+=input[cur];
                     cur--;
                }

                reverse(tab_inp.begin(), tab_inp.end());


                vecs matches;

                for (const auto &entry : fs::directory_iterator(path))
                {
                    string possible_file(entry.path().filename());
                    if (possible_file.substr(0, tab_inp.size()) == tab_inp)
                        matches.push_back(possible_file);
                }


                 sort(all(matches));

                if (matches.size() == 1)
                {
                    for (int i = 0; i < tab_inp.size(); i++)
                        printf("\b \b");
                    printf("%s", matches[0].c_str());
                    for (int i = tab_inp.size(); i < matches[0].size(); i++)
                        input += matches[0][i];
                }
                else if(matches.size()>1){
                    int num_matches = matches.size();
                     int lcp_size = min(matches[0].size(), matches[num_matches - 1].size());
                     int idx = 0;

                     while (idx < lcp_size and matches[0][idx] == matches[num_matches - 1][idx])
                        idx++;

                     lcp_size = idx;
                     string lcp = matches[0].substr(0, lcp_size);

                     for (int i = 0; i < tab_inp.size(); i++)
                        printf("\b \b");

                      printf("%s", lcp.c_str());

                      for (int i = tab_inp.size(); i < lcp.size(); i++)
                        input += lcp[i];

                        printf("\n");

                         for (int i = 0; i < matches.size(); i++)
                        printf("(%d) %s  ", i + 1, matches[i].c_str());
                    printf("\nEnter the choice: ");

                    char choice[10];
                    int choice_idx = 0;
                    char inp_;

                     do
                    {
                        inp_ = getchar();
                        if (inp_ == '\n')
                        {
                            choice[choice_idx] = '\0';
                            printf("\n");
                        }
                        else if (inp_ == 127)
                        {
                            if (choice_idx)
                            {
                                choice_idx--;
                                printf("\b \b");
                            }
                        }
                        else
                        {
                            choice[choice_idx++] = inp_;
                            printf("%c", inp_);
                        }
                    } while (inp_ != '\n');

                    int choosen_file = atoi(choice);
                    cout << BOLD << BLUE << "Shell>> " << RESET;
                    fflush(stdout);

                     if (choosen_file <= 0 or choosen_file > matches.size())
                        choosen_file = 1;

                    for (int i = lcp.size(); i < matches[choosen_file - 1].size(); i++)
                        input += matches[choosen_file - 1][i];

                    cout<<input;
                }
                 
            }  
            else if(inp =='\n'||inp==18){
                printf("\n");
            }
           
            else if(inp==127){
                history_nav=0;  
                if(input.size()>0) {
                    printf("\b \b");
                    input.pop_back();
                }
                if(!input.size()) {
                    history_nav = 1;
                    history_cur = 0;
                }
            }

            else if(inp==27){

                char esc1 = getchar();
                char esc2 = getchar();

                if(esc1==91){
                    switch(esc2){
                    case 65:
                        if (history_nav)
                        {
                            if (history_cur != history::shellHistoy().getSize())
                                history_cur++;
                            for (int i = 0; i < input.size(); i++)
                                printf("\b \b");
                            while(input.size()) input.pop_back();
                            if (history_cur)
                            {
                                for (auto c : history::shellHistoy().getHistory()[history_cur - 1])
                                {
                                    input += c;
                                    printf("%c", c);
                                }
                            }   
                        }
                        break;

                    case 66:
                        if (history_nav)
                        {
                           if (history_cur != 0)
                                history_cur--;
                           for (int i = 0; i < input.size(); i++)
                                printf("\b \b");
                            while(input.size()) input.pop_back();
                            if (history_cur)
                            {
                                for (auto c : history::shellHistoy().getHistory()[history_cur - 1])
                                {
                                    input += c;
                                    printf("%c", c);
                                }
                            }
                        }
                        break;

                    default:
                        break;
                    }
                }

            }
            
            else {
                history_nav = 0;
                input+=inp;
                printf("%c", inp);
            }

        } while(inp!='\n');

        if(input!="" )  history::shellHistoy().addHistory(input);


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
