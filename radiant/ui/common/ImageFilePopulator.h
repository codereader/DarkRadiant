#pragma once

#include "ifilesystem.h"
#include "gamelib.h"
#include "string/case_conv.h"
#include "string/predicate.h"
#include "string/trim.h"
#include "os/path.h"
#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/Bitmap.h"

namespace ui
{

namespace
{
    const char* const TEXTURE_ICON = "icon_texture.png";
    const char* const FOLDER_ICON = "folder16.png";
}

class ImageFileFunctor :
    public wxutil::VFSTreePopulator
{
private:
    const wxutil::ResourceTreeView::Columns& _columns;

    wxIcon _fileIcon;
    wxIcon _folderIcon;

public:
    // Constructor
    ImageFileFunctor(const wxutil::TreeModel::Ptr& treeStore,
        const wxutil::ResourceTreeView::Columns& columns) :
        VFSTreePopulator(treeStore),
        _columns(columns)
    {
        _fileIcon.CopyFromBitmap(wxutil::GetLocalBitmap(TEXTURE_ICON));
        _folderIcon.CopyFromBitmap(wxutil::GetLocalBitmap(FOLDER_ICON));
    }

    void addFile(const vfs::FileInfo& fileInfo)
    {
        const std::string ddsPrefix = "dds/";
        std::string fullPath = fileInfo.fullPath();

        // Sort the shader into the tree and set the values
        addPath(fullPath, [&](wxutil::TreeModel::Row& row, const std::string& path,
            const std::string& leafName, bool isFolder)
        {
            // The map expressions don't need any file extensions
            auto imageFilePath = os::removeExtension(path);

            // Cut off the dds prefix, it won't be valid when set in a material
            if (string::istarts_with(path, ddsPrefix))
            {
                imageFilePath = imageFilePath.substr(ddsPrefix.length());
            }

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, isFolder ? _folderIcon : _fileIcon));
            row[_columns.leafName] = leafName;
            row[_columns.fullName] = imageFilePath;
            row[_columns.isFolder] = isFolder;
            row[_columns.isFavourite] = false;

            row.SendItemAdded();
        });
    }
};

class ImageFilePopulator final :
    public wxutil::ThreadedResourceTreePopulator
{
private:
    const wxutil::ResourceTreeView::Columns& _columns;
    const char* const GKEY_IMAGE_TYPES = "/filetypes/texture//extension";
    std::set<std::string> _extensions;

public:
    // Construct and initialise variables
    ImageFilePopulator(const wxutil::ResourceTreeView::Columns& columns) :
        ThreadedResourceTreePopulator(columns),
        _columns(columns)
    {
        auto texTypes = game::current::getNodes(GKEY_IMAGE_TYPES);

        for (const auto& node : texTypes)
        {
            // Get the file extension, store it as lowercase
            std::string extension = node.getContent();
            _extensions.emplace(string::to_lower_copy(extension));
        }
    }

    ~ImageFilePopulator()
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        ImageFileFunctor functor(model, _columns);

        GlobalFileSystem().forEachFile(
            "", "*",
            [&](const vfs::FileInfo& fileInfo)
            { 
                ThrowIfCancellationRequested();

                if (_extensions.count(string::to_lower_copy(os::getExtension(fileInfo.name))) == 0)
                {
                    return;
                }

                functor.addFile(fileInfo);
            },
            99
        );
    }

    void SortModel(const wxutil::TreeModel::Ptr& model) override
    {
        model->SortModelFoldersFirst(_columns.iconAndName, _columns.isFolder);
    }
};

}
