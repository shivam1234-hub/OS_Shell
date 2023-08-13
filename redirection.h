#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <string>
#include <vector>
using namespace std;

class Redirection {
public:
    static void setupRedirection(vector<string>& args, int& stdin_fd, int& stdout_fd);
};

#endif // REDIRECTION_H