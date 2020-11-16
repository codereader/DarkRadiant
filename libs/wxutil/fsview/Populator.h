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
        filename(add(wxutil::TreeModel::Column::IconText)),
        vfspath(add(wxutil::TreeModel::Column::String)),
        isFolder(add(wxutil::TreeModel::Column::Boolean))
    {}

    wxutil::TreeModel::Column filename;	// e.g. "chair1.pfb"
    wxutil::TreeModel::Column vfspath;	// e.g. "prefabs/chair1.pfb"
    wxutil::TreeModel::Column isFolder;	// whether this is a folder
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

public:
    Populator(const TreeColumns& columns,
        wxEvtHandler* finishedHandler,
        const std::string& basePath);

    ~Populator(); // waits for thread to finish

    // Returns the base path as used in the constructor
    const std::string& GetBasePath() const;

    void Populate();

    void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
        const std::string& path, bool isExplicit);

protected:
    // Thread entry point
    ExitCode Entry();

    void visitFile(const vfs::FileInfo& fileInfo);
    void SearchForFilesMatchingExtension(const std::string& extension);
};

}

}
