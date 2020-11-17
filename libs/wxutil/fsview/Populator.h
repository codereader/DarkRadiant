#pragma once

#include <wx/thread.h>
#include "ifilesystem.h"
#include "wxutil/VFSTreePopulator.h"

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
        size(add(TreeModel::Column::String))
    {}

    TreeModel::Column filename; // e.g. "chair1.pfb"
    TreeModel::Column vfspath;  // e.g. "prefabs/chair1.pfb"
    TreeModel::Column isFolder; // whether this is a folder
    TreeModel::Column size;     // file size string
};

class Populator :
    public wxutil::VFSTreePopulator::Visitor,
    public wxThread
{
private:
    const TreeColumns& _columns;

    // The tree store to populate
    wxutil::TreeModel::Ptr _treeStore;

    // The event handler to notify on completion
    wxEvtHandler* _finishedHandler;

    // The helper class, doing the tedious treeview insertion for us.
    wxutil::VFSTreePopulator _treePopulator;

    wxIcon _fileIcon;
    wxIcon _folderIcon;

    std::string _basePath;

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

    void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
        const std::string& path, bool isExplicit);

    void SetDefaultFileIcon(const std::string& fileIcon);

protected:
    // Thread entry point
    ExitCode Entry();

    void visitFile(const vfs::FileInfo& fileInfo);
    void SearchForFilesMatchingExtension(const std::string& extension);
};

}

}
