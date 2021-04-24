#include "SegFaultHandler.h"

#include "itextstream.h"

#ifdef POSIX
#include <execinfo.h>
#include <signal.h>
#endif

namespace applog
{

void SegFaultHandler::Install()
{
#ifdef POSIX
    signal(SIGSEGV, _handleSigSegv);
#endif
}

void SegFaultHandler::_handleSigSegv(int sig)
{
#ifdef POSIX
    rError() << "SIGSEGV signal caught: " << sig << std::endl;
    std::cerr << "SIGSEGV signal caught: " << sig << std::endl;

    void* buffer[100];
    int numAddresses = backtrace(buffer, sizeof(buffer));
    char** strings = backtrace_symbols(buffer, numAddresses);

    if (strings == nullptr)
    {
        std::cerr << "backtrace() returned nullptr" << std::endl;
        exit(EXIT_FAILURE);
    }

    for (int j = 0; j < numAddresses; j++)
    {
        rError() << j << ": " << strings[j] << std::endl;
        std::cerr << j << ": " << strings[j] << std::endl;
    }

    free(strings);
    exit(1);
#endif
}

}
