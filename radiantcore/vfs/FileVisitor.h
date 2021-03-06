#pragma once

#include <set>

#include "ifilesystem.h"
#include "AssetsList.h"

namespace vfs
{

/*
 * IArchive::Visitor class used in GlobalFileSystem().foreachFile().
 * It's filtering out the files matching the defined extension only.
 * The directory part is cut off the filename.
 * On top of that, this class maintains a list of visited files to avoid
 * hitting the same file twice (it might be present in more than one Archive).
 */
class FileVisitor :
    public IArchive::Visitor
{
    // Maximum traversal depth
    std::size_t _maxDepth;

    // User-supplied functor to invoke for each file
    VirtualFileSystem::VisitorFunc _visitorFunc;

    // Optional AssetsList containing visibility information
    const AssetsList* _assetsList = nullptr;

    // Set of already-visited files to avoid visiting the same file twice
    std::set<std::string> _visitedFiles;

    // Directory to search within
    std::string _directory;

    // Extension to match
    std::string _extension;

    // The length of the directory name
    std::size_t _dirPrefixLength;

    bool _visitAll;

    std::size_t _extLength;

public:

    // Constructor. Pass "*" as extension to have it visit all files.
    FileVisitor(const VirtualFileSystem::VisitorFunc& visitorFunc,
        const std::string& dir, const std::string& ext,
        std::size_t maxDepth) :
        _maxDepth(maxDepth), 
        _visitorFunc(visitorFunc), 
        _directory(dir),
        _extension(ext), 
        _dirPrefixLength(_directory.length()),
        _visitAll(_extension == "*"), 
        _extLength(_extension.length())
    {}

    void setAssetsList(const AssetsList& list)
    {
        _assetsList = &list;
    }

    // Archive::Visitor interface
    void visitFile(const std::string& name, IArchiveFileInfoProvider& infoProvider) override
    {
#ifdef OS_CASE_INSENSITIVE
        // The name should start with the directory, "def/" for instance, case-insensitively.
        assert(string::to_lower_copy(name.substr(0, _dirPrefixLength)) == _directory);
#else
        // Linux: The name should start with the directory, "def/" for instance, including case.
        assert(name.substr(0, _dirPrefixLength) == _directory);
#endif

        // Cut off the base directory prefix
        std::string subname = name.substr(_dirPrefixLength);

        // Check for matching file extension
        if (!_visitAll)
        {
            // The dot must be at the right position
            if (subname.length() <= _extLength || subname[subname.length() - _extLength - 1] != '.')
            {
                return;
            }

            // And the extension must match
            std::string ext = subname.substr(subname.length() - _extLength);

#ifdef OS_CASE_INSENSITIVE
            // Treat extensions case-insensitively in Windows
            string::to_lower(ext);
#endif

            if (ext != _extension)
            {
                return; // extension mismatch
            }
        }

        if (_visitedFiles.find(subname) != _visitedFiles.end())
        {
            return; // already visited
        }

        // Don't return assets.lst itself
        if (subname == AssetsList::FILENAME)
            return;

        // Suitable file, call the callback and add to visited file set
        vfs::Visibility vis = _assetsList ? _assetsList->getVisibility(subname)
            : Visibility::NORMAL;
        _visitorFunc(vfs::FileInfo(_directory, subname, vis, infoProvider));

        _visitedFiles.insert(subname);
    }

    bool visitDirectory(const std::string& name, std::size_t depth) override
    {
        if (depth == _maxDepth)
        {
            return true;
        }

        return false;
    }
};

}
