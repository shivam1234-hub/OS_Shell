#ifndef PIPE_H
#define PIPE_H

#include <vector>
#include <string>
using namespace std;

class PipeHandler {
public:
    static void executePipedCommands(string& command);
};

#endif // PIPE_H
