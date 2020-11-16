#include "Populator.h"

#include "iuimanager.h"
#include "ifiletypes.h"
#include "os/path.h"

#include <wx/artprov.h>

namespace wxutil
{

namespace fsview
{

namespace
{
    const char* const FOLDER_ICON = "folder.png";
    const char* const FILE_ICON = "cmenu_add_prefab.png";
}

Populator::Populator(const TreeColumns& columns,
    wxEvtHandler* finishedHandler, const std::string& basePath) :
    wxThread(wxTHREAD_JOINABLE),
    _columns(columns),
    _treeStore(new wxutil::TreeModel(_columns)),
    _finishedHandler(finishedHandler),
    _treePopulator(_treeStore),
    _basePath(os::standardPathWithSlash(basePath))
{
    _fileIcon.CopyFromBitmap(
        wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FILE_ICON));
    _folderIcon.CopyFromBitmap(
        wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
}

Populator::~Populator()
{
    // We might have a running thread, wait for it
    if (IsRunning())
    {
        Delete();
    }
}

void Populator::visitFile(const vfs::FileInfo& fileInfo)
{
    if (TestDestroy())
    {
        return;
    }

    // Let the VFSTreePopulator do the insertion
    _treePopulator.addPath(fileInfo.name);
}

void Populator::SearchForFilesMatchingExtension(const std::string& extension)
{
    if (path_is_absolute(_basePath.c_str()))
    {
        // Traverse a folder somewhere in the filesystem
        GlobalFileSystem().forEachFileInAbsolutePath(_basePath, extension,
            std::bind(&Populator::visitFile, this, std::placeholders::_1), 0);
    }
    else
    {
        // Traverse the VFS
        GlobalFileSystem().forEachFile(_basePath, extension,
            std::bind(&Populator::visitFile, this, std::placeholders::_1), 0);
    }
}

wxThread::ExitCode Populator::Entry()
{
    // Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
    FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(filetype::TYPE_PREFAB);

    for (const auto& pattern : patterns)
    {
        SearchForFilesMatchingExtension(pattern.extension);

        if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);
    }

    // Visit the tree populator in order to fill in the column data
    _treePopulator.forEachNode(*this);

    if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

    // Sort the model before returning it
    _treeStore->SortModelFoldersFirst(_columns.filename, _columns.isFolder);

    if (!TestDestroy())
    {
        // Send the event to our listener, only if we are not forced to finish
        wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
    }

    return static_cast<wxThread::ExitCode>(0);
}

const std::string& Populator::GetBasePath() const
{
    return _basePath;
}

void Populator::Populate()
{
    if (IsRunning()) return;

    Run();
}

void Populator::visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
    const std::string& path, bool isExplicit)
{
    if (TestDestroy()) return;

    // Get the display path, everything after rightmost slash
    row[_columns.filename] = wxVariant(wxDataViewIconText(path.substr(path.rfind("/") + 1),
        isExplicit ? _fileIcon : _folderIcon));
    row[_columns.vfspath] = _basePath + path;
    row[_columns.isFolder] = !isExplicit;
}

}

}
