#include "Populator.h"

#include "i18n.h"
#include "ifiletypes.h"
#include "iarchive.h"
#include "os/path.h"
#include "string/case_conv.h"
#include "string/format.h"
#include "gamelib.h"

#include "wxutil/Bitmap.h"
#include <fmt/format.h>

namespace wxutil
{

namespace fsview
{

namespace
{
    const char* const FOLDER_ICON = "folder.png";
    const char* const FILE_ICON = "file.png";
}

Populator::Populator(const TreeColumns& columns,
    wxEvtHandler* finishedHandler, const std::string& basePath, 
    const std::set<std::string>& fileExtensions) :
    wxThread(wxTHREAD_JOINABLE),
    _columns(columns),
    _basePath(basePath),
    _treeStore(new wxutil::TreeModel(_columns)),
    _finishedHandler(finishedHandler),
    _treePopulator(_treeStore),
    _fileExtensions(fileExtensions)
{
    _fileIcon.CopyFromBitmap(
        wxutil::GetLocalBitmap(FILE_ICON));
    _folderIcon.CopyFromBitmap(
        wxutil::GetLocalBitmap(FOLDER_ICON));

    _basePathItem = insertBasePathItem();
    _treePopulator.setTopLevelItem(_basePathItem);
}

Populator::~Populator()
{
    // We might have a running thread, wait for it
    if (IsRunning())
    {
        Delete();
    }
}

void Populator::SetDefaultFileIcon(const std::string& fileIcon)
{
    _fileIcon.CopyFromBitmap(wxutil::GetLocalBitmap(fileIcon));
}

void Populator::visitFile(const vfs::FileInfo& fileInfo)
{
    if (TestDestroy())
    {
        return;
    }

    // Let the VFSTreePopulator do the insertion
    _treePopulator.addPath(fileInfo.name, [&](TreeModel::Row& row, 
        const std::string& path, const std::string& leafName,
        bool isFolder)
    {
        // The population callback will be called multiple times for deeper files,
        // but only one of them will be have isFolder == false, which is our actual file
        std::string vfsPath = _rootPath + path;

        row[_columns.filename] = wxVariant(wxDataViewIconText(leafName,
            isFolder ? _folderIcon : GetIconForFile(leafName)));
        row[_columns.vfspath] = isFolder ? os::standardPathWithSlash(vfsPath) : vfsPath;
        row[_columns.isFolder] = isFolder;

        if (!isFolder)
        {
            // Get the file size if possible
            bool isPhysical = fileInfo.getIsPhysicalFile();
            auto archivePath = fileInfo.getArchivePath();

            row[_columns.size] = string::getFormattedByteSize(fileInfo.getSize());
            row[_columns.isPhysical] = isPhysical;
            row[_columns.archivePath] = archivePath;

            if (!isPhysical)
            {
                row[_columns.archiveDisplay] = fmt::format(_("[in {0}]"), os::getFilename(archivePath));
            }
        }
    });
}

void Populator::SearchForFilesMatchingExtension(const std::string& extension)
{
    if (path_is_absolute(_basePath.c_str()))
    {
        if (os::isDirectory(_basePath))
        {
            _rootPath = os::standardPathWithSlash(_basePath);

            // Traverse a folder somewhere in the filesystem
            GlobalFileSystem().forEachFileInAbsolutePath(os::standardPathWithSlash(_basePath), extension,
                std::bind(&Populator::visitFile, this, std::placeholders::_1), 0);
        }
        else 
        {
            _rootPath = "";

            // Try to open this file as archive
            GlobalFileSystem().forEachFileInArchive(_basePath, extension,
                std::bind(&Populator::visitFile, this, std::placeholders::_1), 0);
        }
    }
    else
    {
        _rootPath = os::standardPathWithSlash(_basePath);

        // Traverse the VFS using that root path
        GlobalFileSystem().forEachFile(_rootPath, extension,
            std::bind(&Populator::visitFile, this, std::placeholders::_1), 0);
    }
}

wxThread::ExitCode Populator::Entry()
{
    for (const auto& extension : _fileExtensions)
    {
        SearchForFilesMatchingExtension(extension);

        if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);
    }

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

const wxIcon& Populator::GetIconForFile(const std::string& path)
{
    auto extension = string::to_lower_copy(os::getExtension(path));
    auto foundIcon = _iconsPerExtension.find(extension);

    if (foundIcon != _iconsPerExtension.end())
    {
        return foundIcon->second;
    }

    // Try to find an icon in the file type registry
    auto iconName = GlobalFiletypes().getIconForExtension(extension);

    if (!iconName.empty())
    {
        wxIcon customIcon;
        customIcon.CopyFromBitmap(wxutil::GetLocalBitmap(iconName));

        foundIcon = _iconsPerExtension.emplace(extension, customIcon).first;
    }
    else
    {
        // No special icon, use the default file icon
        foundIcon = _iconsPerExtension.emplace(extension, _fileIcon).first;
    }

    return foundIcon->second;
}

wxDataViewItem Populator::insertBasePathItem()
{
    auto row = _treeStore->AddItem();
    row[_columns.filename] = _basePath;
    row[_columns.vfspath] = _basePath;
    row[_columns.isFolder] = true;

    std::string realBasePath = _basePath;

    if (!path_is_absolute(_basePath.c_str()))
    {
        // Prepend the mod name to the base path
        realBasePath = fmt::format("{0}:{1}", 
            GlobalGameManager().currentGame()->getKeyValue("name"),
            !_basePath.empty() ? _basePath : "/");
    }

    bool basePathIsFolder = os::isDirectory(realBasePath);
    row[_columns.filename] = wxVariant(wxDataViewIconText(realBasePath,
        basePathIsFolder ? _folderIcon : GetIconForFile(realBasePath)));

    return row.getItem();
}

}

}
