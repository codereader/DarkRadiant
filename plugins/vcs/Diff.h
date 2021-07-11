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

    bool containsFile(const std::string& relativePath)
    {
        FileSearch search{ relativePath, false };
        git_diff_foreach(_diff, Diff::searchForFile, nullptr, nullptr, nullptr, &search);

        return search.found;
    }

private:
    struct FileSearch
    {
        std::string path;
        bool found;
    };
    
    static int searchForFile(const git_diff_delta* delta, float progress, void* payload)
    {
        auto* search = reinterpret_cast<FileSearch*>(payload);

        if (delta->new_file.path == search->path)
        {
            search->found = true;
        }

        return 0;
    }
};

}

}
