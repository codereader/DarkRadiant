#pragma once

namespace applog
{

// SIGSEGV Signal handler to write some information to the console and the error log
// in case the application crashes.
class SegFaultHandler
{
public:
    static void Install();

private:
    static void _handleSigSegv(int sig);
};

}
