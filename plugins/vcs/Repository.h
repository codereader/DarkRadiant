#pragma once

#include <string>
#include "itextstream.h"
#include "git2.h"

namespace vcs
{

namespace git
{

/**
 * Represents a Git repository at a certain path
 */
class Repository
{
private:
    git_repository* _repository;
    bool _isOk;

public:
    Repository(const std::string& path) :
        _repository(nullptr),
        _isOk(false)
    {
        if (git_repository_open(&_repository, path.c_str()) == 0)
        {
            rMessage() << "Opened repository at " << path << std::endl;
            _isOk = true;
        }
        else
        {
            rMessage() << "Failed to open repository at " << path << std::endl;
        }
    }

    // Status query of this repository object
    bool isOk() const
    {
        return _isOk;
    }

    ~Repository()
    {
        git_repository_free(_repository);
    }
};

}

}
