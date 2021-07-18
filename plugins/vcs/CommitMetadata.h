#pragma once

#include <string>

namespace vcs
{

namespace git
{

struct CommitMetadata
{
    std::string name;
    std::string email;
    std::string message;

    bool isValid() const
    {
        return !name.empty() && !email.empty() && !message.empty();
    }
};

}

}
