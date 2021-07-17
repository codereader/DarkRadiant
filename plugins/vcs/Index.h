#pragma once

#include <memory>
#include <git2.h>

namespace vcs
{

namespace git
{

class Index final
{
private:
    git_index* _index;

public:
    using Ptr = std::shared_ptr<Index>;
    
    Index(git_index* index) :
        _index(index)
    {}

    ~Index()
    {
        git_index_free(_index);
    }

    bool hasConflicts()
    {
        return git_index_has_conflicts(_index);
    }
};

}

}
