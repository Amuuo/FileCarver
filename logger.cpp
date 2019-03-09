#include "logger.h"

Logger::Logger(){}

 void Logger::LaunchLogWindow()
{
    char *name = tempnam(NULL, NULL);
    char cmd[256];

    mkfifo(name, 0600);
    if(fork() == 0)
    {
    sprintf(cmd, "xterm -e cat %s", name);
    system(cmd);
    exit(0);
    }
    outputFile.open(name);
}


Logger& Logger::operator<<(const char* in){
    outputFile << in;
    outputFile.flush();
}