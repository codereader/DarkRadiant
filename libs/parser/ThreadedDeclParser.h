#pragma once

#include "ifilesystem.h"
#include "ThreadedDefLoader.h"

namespace parser
{

/**
 * Threaded declaration parser, visiting all files associated to the given
 * decl type, processing the files in the correct order.
 */
template <typename ReturnType>
class ThreadedDeclParser :
    public util::ThreadedDefLoader<ReturnType>
{
public:
    using LoadFunction = util::ThreadedDefLoader<ReturnType>::LoadFunction;

private:
    std::string _baseDir;
    std::string _extension;
    std::size_t _depth;

public:
    ThreadedDeclParser(const std::string& baseDir, const std::string& extension, 
                       const LoadFunction& loadFunc) :
        ThreadedDeclParser(baseDir, extension, 0, loadFunc)
    {}

    ThreadedDeclParser(const std::string& baseDir, const std::string& extension, std::size_t depth,
                       const LoadFunction& loadFunc) :
        util::ThreadedDefLoader<typename ReturnType>(loadFunc),
        _baseDir(baseDir),
        _extension(extension),
        _depth(depth)
    {}

    virtual ~ThreadedDeclParser()
    {}

protected:
    void loadFiles(const vfs::VirtualFileSystem::VisitorFunc& visitor)
    {
        loadFiles(GlobalFileSystem(), visitor);
    }

    void loadFiles(vfs::VirtualFileSystem& vfs, const vfs::VirtualFileSystem::VisitorFunc& visitor)
    {
        // Accumulate all the files and sort them before calling the visitor
        std::vector<vfs::FileInfo> _incomingFiles;
        _incomingFiles.reserve(200);

        vfs.forEachFile(_baseDir, _extension, [&](const vfs::FileInfo& info)
        {
            _incomingFiles.push_back(info);
        }, _depth);

        // Sort the files by name
        std::sort(_incomingFiles.begin(), _incomingFiles.end(), [](const vfs::FileInfo& a, const vfs::FileInfo& b)
        {
            return a.name < b.name;
        });

        // Dispatch the sorted list to the visitor
        std::for_each(_incomingFiles.begin(), _incomingFiles.end(), visitor);
    }
};

}
