#pragma once

#include <string>
#include <memory>

struct git_repository;

namespace vcs
{

namespace git
{

class Remote;

/**
 * Represents a Git repository at a certain path
 */
class Repository final
{
private:
    git_repository* _repository;
    bool _isOk;

public:
    Repository(const std::string& path);
    ~Repository();

    // Status query of this repository object, 
    // returns true if this repository exists and has been successfully opened
    bool isOk() const;

    // Returns the remote with the given name
    std::shared_ptr<Remote> getRemote(const std::string& name);

    std::string getCurrentBranchName();

    // Return the raw libgit2 object
    git_repository* _get();
};

}

}
