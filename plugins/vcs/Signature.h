#pragma once

#include <git2.h>

#include "GitException.h"

namespace vcs
{

namespace git
{

class Signature final
{
private:
    git_signature* _signature;
public:
    Signature(const std::string& name, const std::string& email) :
        _signature(nullptr)
    {
        auto error = git_signature_now(&_signature, name.c_str(), email.c_str());
        GitException::ThrowOnError(error);
    }

    git_signature* get()
    {
        return _signature;
    }

    ~Signature()
    {
        if (_signature != nullptr)
        {
            git_signature_free(_signature);
        }
    }
};

}

}
