#pragma once

#include <wx/thread.h>
#include "ifilesystem.h"
#include "wxutil/Icon.h"
#include "wxutil/dataview/VFSTreePopulator.h"

namespace wxutil
{

namespace fsview
{

// Treemodel definition
struct TreeColumns :
    public TreeModel::ColumnRecord
{
    TreeColumns() :
        filename(add(TreeModel::Column::IconText)),
        vfspath(add(TreeModel::Column::String)),
        isFolder(add(TreeModel::Column::Boolean)),
        size(add(TreeModel::Column::String)),
        isPhysical(add(TreeModel::Column::Boolean)),
        archivePath(add(TreeModel::Column::String)),
        archiveDisplay(add(TreeModel::Column::String))
    {}

    TreeModel::Column filename;    // e.g. "chair1.pfb"
    TreeModel::Column vfspath;     // e.g. "prefabs/chair1.pfb"
    TreeModel::Column isFolder;    // whether this is a folder
    TreeModel::Column size;        // file size string
    TreeModel::Column isPhysical;  // file size string
    TreeModel::Column archivePath;  // path to containing archive
    TreeModel::Column archiveDisplay;  // string to display the parent container
};

class Populator :
    public wxThread
{
private:
    const TreeColumns& _columns;

    // The path to inspect, which can be either a VFS-relative path
    // (or even an empty string for the VFS root) or an absolute path
    // that points to a physical directory or a PAK file. 
    std::string _basePath;

    // Will be preprended to each item's VFS path
    // When inspecting physical PK4s, this will be an empty string
    // such that the VFS path of each item shows up as relative path
    std::string _rootPath;

    // The tree store to populate
    wxutil::TreeModel::Ptr _treeStore;

    wxDataViewItem _basePathItem;

    // The event handler to notify on completion
    wxEvtHandler* _finishedHandler;

    // The helper class, doing the tedious treeview insertion for us.
    wxutil::VFSTreePopulator _treePopulator;

    wxutil::Icon _fileIcon;
    wxutil::Icon _folderIcon;
    std::map<std::string, wxutil::Icon> _iconsPerExtension;

    std::set<std::string> _fileExtensions;

public:
    Populator(const TreeColumns& columns,
        wxEvtHandler* finishedHandler,
        const std::string& basePath,
        const std::set<std::string>& fileExtensions);

    ~Populator(); // waits for thread to finish

    // Returns the base path as used in the constructor
    const std::string& GetBasePath() const;

    void Populate();

    void SetDefaultFileIcon(const std::string& fileIcon);

protected:
    // Thread entry point
    ExitCode Entry();

    void visitFile(const vfs::FileInfo& fileInfo);
    void SearchForFilesMatchingExtension(const std::string& extension);
    const wxutil::Icon& GetIconForFile(const std::string& path);
    wxDataViewItem insertBasePathItem();
};

}

}
