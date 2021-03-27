#include "MaterialPopulator.h"

#include <map>
#include "ifavourites.h"
#include "i18n.h"
#include "ishaders.h"

#include "string/string.h"
#include "shaderlib.h"

#include "wxutil/Bitmap.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

namespace ui
{

namespace
{
    const char* const OTHER_MATERIALS_FOLDER = N_("Other Materials");

    constexpr const char* const FOLDER_ICON = "folder16.png";
    constexpr const char* const TEXTURE_ICON = "icon_texture.png";
}

struct ShaderNameFunctor
{
    // TreeStore to populate
    wxutil::TreeModel& _store;
    const MaterialTreeView::TreeColumns& _columns;
    const std::set<std::string>& _favourites;
    wxDataViewItem _root;

    std::string _otherMaterialsPath;

    // Maps of names to corresponding treemodel items, for both intermediate
    // paths and explicitly presented paths
    using NamedIterMap = std::map<std::string, wxDataViewItem, string::ILess>;
    NamedIterMap _iters;

    wxIcon _folderIcon;
    wxIcon _textureIcon;

    ShaderNameFunctor(wxutil::TreeModel& store, const MaterialTreeView::TreeColumns& columns, const std::set<std::string>& favourites) :
        _store(store),
        _columns(columns),
        _favourites(favourites),
        _root(_store.GetRoot()),
        _otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
    {
        _folderIcon.CopyFromBitmap(wxutil::GetLocalBitmap(FOLDER_ICON));
        _textureIcon.CopyFromBitmap(wxutil::GetLocalBitmap(TEXTURE_ICON));
    }

    // Recursive add function
    wxDataViewItem& addRecursive(const std::string& path, bool isOtherMaterial)
    {
        // Look up candidate in the map and return it if found
        auto it = _iters.find(path);

        if (it != _iters.end())
        {
            return it->second;
        }

        /* Otherwise, split the path on its rightmost slash, call recursively on the
            * first half in order to add the parent node, then add the second half as
            * a child. Recursive bottom-out is when there is no slash (top-level node).
            */
            // Find rightmost slash
        std::size_t slashPos = path.rfind("/");

        // Call recursively to get parent iter, leaving it at the toplevel if
        // there is no slash
        wxDataViewItem& parIter =
            slashPos != std::string::npos ? addRecursive(path.substr(0, slashPos), isOtherMaterial) : _root;

        // Append a node to the tree view for this child
        wxutil::TreeModel::Row row = _store.AddItem(parIter);

        std::string name = slashPos != std::string::npos ? path.substr(slashPos + 1) : path;

        row[_columns.iconAndName] = wxVariant(wxDataViewIconText(name, _folderIcon));
        row[_columns.leafName] = name;
        row[_columns.fullName] = path;
        row[_columns.isFolder] = true;
        row[_columns.isOtherMaterialsFolder] = isOtherMaterial;
        row[_columns.isFavourite] = false; // folders are not favourites

        // Add a copy of the wxDataViewItem to our hashmap and return it
        std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
            NamedIterMap::value_type(path, row.getItem()));

        return result.first->second;
    }

    void visit(const std::string& name)
    {
        // Find rightmost slash
        std::size_t slashPos = name.rfind("/");

        wxDataViewItem parent;

        if (string::istarts_with(name, GlobalTexturePrefix_get()))
        {
            // Regular texture, ensure parent folder
            parent = slashPos != std::string::npos ? addRecursive(name.substr(0, slashPos), false) : _root;
        }
        else
        {
            // Put it under "other materials", ensure parent folder
            parent = slashPos != std::string::npos ?
                addRecursive(_otherMaterialsPath + "/" + name.substr(0, slashPos), true) :
                addRecursive(_otherMaterialsPath, true);
        }

        // Insert the actual leaf
        wxutil::TreeModel::Row row = _store.AddItem(parent);

        std::string leafName = slashPos != std::string::npos ? name.substr(slashPos + 1) : name;

        bool isFavourite = _favourites.count(name) > 0;

        row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon));
        row[_columns.leafName] = leafName;
        row[_columns.fullName] = name;
        row[_columns.isFolder] = false;
        row[_columns.isOtherMaterialsFolder] = false;
        row[_columns.isFavourite] = isFavourite;

        // Formatting
        row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
    }
};

// Construct and initialise variables
MaterialPopulator::MaterialPopulator(const MaterialTreeView::TreeColumns& columns) :
    ThreadedResourceTreePopulator(columns),
    _columns(columns)
{
    _favourites = GlobalFavouritesManager().getFavourites(decl::Type::Material);
}

MaterialPopulator::~MaterialPopulator()
{
    // Stop the worker while the class is still intact
    EnsureStopped();
}

void MaterialPopulator::AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName)
{
    ShaderNameFunctor functor(*model, _columns, _favourites);
    functor.visit(materialName);
}

void MaterialPopulator::RemoveSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName)
{
    auto item = model->FindString(materialName, _columns.fullName);
    if (!item.IsOk()) return;

    model->RemoveItem(item);
}

void MaterialPopulator::PopulateModel(const wxutil::TreeModel::Ptr& model)
{
    model->SetHasDefaultCompare(false);

    ShaderNameFunctor functor(*model, _columns, _favourites);
    GlobalMaterialManager().foreachShaderName([&](const std::string& name)
    {
        ThrowIfCancellationRequested();
        functor.visit(name);
    });
}

void MaterialPopulator::SortModel(const wxutil::TreeModel::Ptr& model)
{
    // Sort the model while we're still in the worker thread
    model->SortModelFoldersFirst(_columns.iconAndName, _columns.isFolder,
        [&](const wxDataViewItem& a, const wxDataViewItem& b)
    {
        // Special folder comparison function
        // A and B are both folders
        wxVariant aIsOtherMaterialsFolder, bIsOtherMaterialsFolder;

        model->GetValue(aIsOtherMaterialsFolder, a, _columns.isOtherMaterialsFolder.getColumnIndex());
        model->GetValue(bIsOtherMaterialsFolder, b, _columns.isOtherMaterialsFolder.getColumnIndex());

        // Special treatment for "Other Materials" folder, which always comes last
        if (aIsOtherMaterialsFolder)
        {
            return +1;
        }

        if (bIsOtherMaterialsFolder)
        {
            return -1;
        }

        return 0; // no special folders, return equal to continue the regular sort algorithm
    });
}

}
