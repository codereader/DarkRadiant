#include "ImageFilePopulator.h"

#include "ifilesystem.h"
#include "gamelib.h"
#include "string/case_conv.h"
#include "string/predicate.h"
#include "string/trim.h"
#include "os/path.h"
#include "wxutil/Bitmap.h"
#include "wxutil/Icon.h"

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
    const ImageFileSelector::Columns& _columns;

    wxutil::Icon _fileIcon;
    wxutil::Icon _folderIcon;

    std::set<std::string> _cubemapSuffixes;

public:
    // Constructor
    ImageFileFunctor(const wxutil::TreeModel::Ptr& treeStore,
        const ImageFileSelector::Columns& columns) :
        VFSTreePopulator(treeStore),
        _columns(columns),
        _cubemapSuffixes({ "_nx", "_ny", "_nz", "_px", "_py", "_pz",
            "_forward", "_back", "_left", "_right", "_up", "_down" }),
        _fileIcon(wxutil::GetLocalBitmap(TEXTURE_ICON)),
        _folderIcon(wxutil::GetLocalBitmap(FOLDER_ICON))
    {}

    void addFile(const vfs::FileInfo& fileInfo)
    {
        const std::string ddsPrefix = "dds/";
        std::string fullPath = fileInfo.fullPath();

        // Sort into the tree and set the values
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

            bool isCubeMapTexture = false;

            // For cubemaps, cut off the suffixes
            if (string::istarts_with(imageFilePath, "env/"))
            {
                auto underscorePos = imageFilePath.find_last_of('_');

                if (underscorePos != std::string::npos)
                {
                    auto suffix = imageFilePath.substr(underscorePos);
                    string::to_lower(suffix);

                    if (_cubemapSuffixes.count(suffix) != 0)
                    {
                        imageFilePath = imageFilePath.substr(0, imageFilePath.length() - suffix.length());
                        isCubeMapTexture = true;
                    }
                }
            }

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, isFolder ? _folderIcon : _fileIcon));
            row[_columns.leafName] = leafName;
            row[_columns.fullName] = imageFilePath;
            row[_columns.isFolder] = isFolder;
            row[_columns.isFavourite] = false;
            row[_columns.isCubeMapTexture] = isCubeMapTexture;

            row.SendItemAdded();
        });
    }
};

ImageFilePopulator::ImageFilePopulator(const ImageFileSelector::Columns& columns) :
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

ImageFilePopulator::~ImageFilePopulator()
{
    EnsureStopped();
}

void ImageFilePopulator::PopulateModel(const wxutil::TreeModel::Ptr& model)
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

void ImageFilePopulator::SortModel(const wxutil::TreeModel::Ptr& model)
{
    model->SortModelFoldersFirst(_columns.iconAndName, _columns.isFolder);
}

}
