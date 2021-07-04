#include "Repository.h"

#include <git2.h>
#include "itextstream.h"
#include "Remote.h"

namespace vcs
{

namespace git
{

Repository::Repository(const std::string& path) :
    _repository(nullptr),
    _isOk(false)
{
    if (git_repository_open(&_repository, path.c_str()) == 0)
    {
        _isOk = true;
    }
    else
    {
        rMessage() << "Failed to open repository at " << path << std::endl;
    }
}

bool Repository::isOk() const
{
    return _isOk;
}

Repository::~Repository()
{
    git_repository_free(_repository);
}

std::shared_ptr<Remote> Repository::getRemote(const std::string& name)
{
    return Remote::CreateFromName(*this, name);
}

git_repository* Repository::_get()
{
    return _repository;
}

}

}
