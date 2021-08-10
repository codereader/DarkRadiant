#pragma once

#include <stdexcept>
#include <git2.h>
#include "itextstream.h"

namespace vcs
{

namespace git
{

class GitException :
    public std::runtime_error
{
public:
    GitException(const std::string& message) :
        runtime_error(GetLastErrorMessage() + "\n" + message)
    {
        rError() << "Git Exception: " << what() << std::endl;
    }

    GitException(int gitErrorCode) :
        runtime_error(GetLastErrorMessage())
    {
        rError() << "Git Exception: " << what() << std::endl;
    }

    static std::string GetLastErrorMessage()
    {
        auto error = git_error_last();

        return error != nullptr ? error->message : "";
    }

    static void ThrowOnError(int errorCode)
    {
        if (errorCode == 0) return;

        throw GitException(errorCode);
    }
};

}

}
