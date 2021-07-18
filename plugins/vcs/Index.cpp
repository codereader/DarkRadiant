#include "Index.h"

#include "GitException.h"
#include "Tree.h"
#include "Repository.h"

namespace vcs
{

namespace git
{

Index::Index(git_index* index) :
    _index(index)
{}

Index::~Index()
{
    git_index_free(_index);
}

std::shared_ptr<Tree> Index::writeTree(Repository& repository)
{
    git_oid treeOid;
    auto error = git_index_write_tree(&treeOid, _index);
    GitException::ThrowOnError(error);

    git_tree* tree;
    error = git_tree_lookup(&tree, repository._get(), &treeOid);
    GitException::ThrowOnError(error);

    return std::make_shared<Tree>(tree);
}

void Index::addAll()
{
    std::string pathSpec("*");
    char* pathSpecCstr = pathSpec.data();

    const git_strarray pathSpecs = {
        &pathSpecCstr,
        1
    };

    auto error = git_index_add_all(_index, &pathSpecs, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);
    GitException::ThrowOnError(error);
}

void Index::updateAll()
{
    std::string pathSpec("*");
    char* pathSpecCstr = pathSpec.data();

    const git_strarray pathSpecs = {
        &pathSpecCstr,
        1
    };

    auto error = git_index_update_all(_index, &pathSpecs, nullptr, nullptr);
    GitException::ThrowOnError(error);
}

void Index::write()
{
    auto error = git_index_write(_index);
    GitException::ThrowOnError(error);
}

bool Index::hasConflicts()
{
    return git_index_has_conflicts(_index);
}

}

}
