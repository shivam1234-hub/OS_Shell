#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

class SignalHandler {
public:
    static void setupSignalHandlers();
    static void handleCtrlC(int signal);
    static void handleCtrlZ(int signal);
};

#endif // SIGNAL_HANDLER_H
