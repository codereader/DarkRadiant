#pragma once

#include <string>
#include <memory>
#include <git2.h>

namespace vcs
{

namespace git
{

class Reference final
{
private:
    git_reference* _reference;

public:
    using Ptr = std::shared_ptr<Reference>;

    Reference(git_reference* reference) :
        _reference(reference)
    {}

    std::string getName() const
    {
        return git_reference_name(_reference);
    }

    std::string getShorthandName() const
    {
        return git_reference_shorthand(_reference);
    }

    // Returns the upstream of this reference (if configured)
    Ptr getUpstream()
    {
        git_reference* upstream = nullptr;
        git_branch_upstream(&upstream, _reference);

        return upstream != nullptr ? std::make_shared<Reference>(upstream) : Ptr();
    }

    ~Reference()
    {
        git_reference_free(_reference);
    }
};

}

}
