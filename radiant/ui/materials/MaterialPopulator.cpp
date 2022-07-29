#include "MaterialPopulator.h"

#include <map>
#include "ifavourites.h"
#include "i18n.h"
#include "ishaders.h"

#include "string/string.h"
#include "string/split.h"
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

// Construct and initialise variables
MaterialPopulator::MaterialPopulator(const MaterialTreeView::TreeColumns& columns) :
    ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON),
    _columns(columns),
    _otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
{
    _folderIcon.CopyFromBitmap(wxutil::GetLocalBitmap(FOLDER_ICON));
    _textureIcon.CopyFromBitmap(wxutil::GetLocalBitmap(TEXTURE_ICON));
}

MaterialPopulator::~MaterialPopulator()
{
    // Stop the worker while the class is still intact
    EnsureStopped();
}

void MaterialPopulator::AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName)
{
    std::vector<std::string> parts;
    string::split(parts, materialName, "/");

    bool isOtherMaterial = parts.size() > 1 && !string::istarts_with(materialName, GlobalTexturePrefix_get());
    auto otherMaterialsFolder = _(OTHER_MATERIALS_FOLDER);

    if (isOtherMaterial)
    {
        parts.insert(parts.begin(), otherMaterialsFolder);
    }

    while (parts.back().empty())
    {
        parts.pop_back();
    }

    wxDataViewItem parentItem;
    std::string parentPath;

    // Ensure all the parent folders are present
    for (auto i = 0; i < parts.size() - 1; ++i)
    {
        if (parts[i].empty())
        {
            continue;
        }

        parentPath += !parentPath.empty() ? "/" : "";
        parentPath += parts[i];

        auto existingItem = model->FindString(parentPath, _columns.fullName, parentItem);

        if (!existingItem.IsOk())
        {
            // Insert this folder
            auto row = insertFolder(model, parentPath, parts[i], parentItem, i == 0 && parts[i] == otherMaterialsFolder);
            row.SendItemAdded();

            parentItem = row.getItem();
        }
        else
        {
            parentItem = existingItem;
        }
    }

    // Insert the material leaf (but don't insert dupes)
    std::string itemPath = parentPath;
    itemPath += !itemPath.empty() ? "/" : "";
    itemPath += parts.back();

    auto existingItem = model->FindString(itemPath, _columns.fullName, parentItem);

    if (!existingItem.IsOk())
    {
        auto row = insertTexture(model, itemPath, parts.back(), parentItem);
        row.SendItemAdded();
    }
}

void MaterialPopulator::RemoveSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName)
{
    auto normalisedName = string::replace_all_copy(materialName, "//", "/");

    // Walk up the parents to check if the removed material leaves any empty folders behind
    auto item = model->FindString(normalisedName, _columns.fullName);
    
    // Prevent removal of items with children
    wxDataViewItemArray children;
    while (item.IsOk() && model->GetChildren(item, children) == 0)
    {
        auto parentItem = model->GetParent(item);

        model->RemoveItem(item);

        if (!parentItem.IsOk())
        {
            break;
        }

        item = parentItem; // remove the parent too (if it is empty)
    }
}

void MaterialPopulator::PopulateModel(const wxutil::TreeModel::Ptr& model)
{
    model->SetHasDefaultCompare(false);

    GlobalMaterialManager().foreachShaderName([&](const std::string& name)
    {
        ThrowIfCancellationRequested();
        insert(model, name);
    });
}

wxutil::TreeModel::Row MaterialPopulator::insertFolder(const wxutil::TreeModel::Ptr& model, 
    const std::string& path, const std::string& leafName, const wxDataViewItem& parentItem, bool isOtherMaterial)
{
    // Append a node to the tree view for this child
    auto row = model->AddItem(parentItem);

    row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _folderIcon));
    row[_columns.leafName] = leafName;
    row[_columns.fullName] = path;
    row[_columns.isFolder] = true;
    row[_columns.isOtherMaterialsFolder] = isOtherMaterial;
    row[_columns.isFavourite] = false; // folders are not favourites

    return row;
}

wxutil::TreeModel::Row MaterialPopulator::insertTexture(const wxutil::TreeModel::Ptr& model,
    const std::string& path, const std::string& leafName, const wxDataViewItem& parentItem)
{
    auto row = model->AddItem(parentItem);

    bool isFavourite = IsFavourite(path);

    row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _textureIcon));
    row[_columns.leafName] = leafName;
    row[_columns.fullName] = path;
    row[_columns.isFolder] = false;
    row[_columns.declName] = path;
    row[_columns.isOtherMaterialsFolder] = false;
    row[_columns.isFavourite] = isFavourite;

    // Formatting
    row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);

    return row;
}

// Recursive add function
wxDataViewItem& MaterialPopulator::addRecursive(const wxutil::TreeModel::Ptr& model,
    const std::string& path, bool isOtherMaterial)
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
    const auto& parIter = slashPos != std::string::npos ? 
        addRecursive(model, path.substr(0, slashPos), isOtherMaterial) : model->GetRoot();

    std::string name = slashPos != std::string::npos ? path.substr(slashPos + 1) : path;

    auto row = insertFolder(model, path, name, parIter, isOtherMaterial);

    // Add a copy of the wxDataViewItem to our hashmap and return it
    auto result = _iters.emplace(path, row.getItem());

    return result.first->second;
}

void MaterialPopulator::insert(const wxutil::TreeModel::Ptr& model, const std::string& name)
{
    // Find rightmost slash
    std::size_t slashPos = name.rfind("/");

    wxDataViewItem parent;

    if (string::istarts_with(name, GlobalTexturePrefix_get()))
    {
        // Regular texture, ensure parent folder
        parent = slashPos != std::string::npos ? addRecursive(model, name.substr(0, slashPos), false) : model->GetRoot();
    }
    else
    {
        // Put it under "other materials", ensure parent folder
        parent = slashPos != std::string::npos ?
            addRecursive(model, _otherMaterialsPath + "/" + name.substr(0, slashPos), true) :
            addRecursive(model, _otherMaterialsPath, true);
    }

    // Insert the actual leaf
    auto leafName = slashPos != std::string::npos ? name.substr(slashPos + 1) : name;
    insertTexture(model, name, leafName, parent);
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
