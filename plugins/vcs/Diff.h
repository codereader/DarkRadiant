#pragma once

#include <git2.h>

namespace vcs
{

namespace git
{

// Represents a set of changes between two states of a repository
class Diff final
{
private:
    git_diff* _diff;

public:
    Diff(git_diff* diff) :
        _diff(diff)
    {}

    ~Diff()
    {
        git_diff_free(_diff);
    }
};

}

}
