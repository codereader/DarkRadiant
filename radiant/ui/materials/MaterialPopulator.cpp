#include "MaterialPopulator.h"

#include "i18n.h"
#include "ishaders.h"
#include "string/replace.h"

#include "string/split.h"

#include "wxutil/dataview/VFSTreePopulator.h"

namespace ui
{

namespace
{
    const char* const OTHER_MATERIALS_FOLDER = N_("Other Materials");

    constexpr const char* const TEXTURE_ICON = "icon_texture.png";
}

MaterialPopulator::MaterialPopulator(const MaterialTreeView::TreeColumns& columns) :
    ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON),
    _columns(columns),
    _texturePrefix(GlobalMaterialManager().getTexturePrefix()),
    _otherMaterialsPath(_(OTHER_MATERIALS_FOLDER))
{}

MaterialPopulator::~MaterialPopulator()
{
    // Stop the worker while the class is still intact
    EnsureStopped();
}

std::string MaterialPopulator::GetOtherMaterialsName()
{
    return _(OTHER_MATERIALS_FOLDER);
}

void MaterialPopulator::AddSingleMaterial(const wxutil::TreeModel::Ptr& model, const std::string& materialName)
{
    std::vector<std::string> parts;
    string::split(parts, materialName, "/");

    bool isOtherMaterial = parts.size() > 1 && !string::istarts_with(materialName, _texturePrefix);

    if (isOtherMaterial)
    {
        parts.insert(parts.begin(), _otherMaterialsPath);
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
            auto row = InsertFolder(model, parentPath, parts[i], parentItem, i == 0 && parts[i] == _otherMaterialsPath);

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
        InsertTexture(model, itemPath, materialName, parts.back(), parentItem);

        // Sort the subtree starting from this parent item
        SortModel(model, parentItem);

        // Force a reload of this subtree by sending events for each child
        model->SendSubtreeRefreshEvents(parentItem);
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

    wxutil::VFSTreePopulator populator(model);

    // Insert the "Other Materials" folder in any case
    populator.addPath(_otherMaterialsPath, [&](wxutil::TreeModel::Row& row,
        const std::string& path, const std::string& leafName, bool isFolder)
    {
        row[_columns.isOtherMaterialsFolder] = true;
        AssignValuesToRow(row, path, path, path, true);
    });

    GlobalMaterialManager().foreachShaderName([&](const std::string& name)
    {
        ThrowIfCancellationRequested();

        // Determine the folder this texture will be sorted into
        auto texturePath = string::istarts_with(name, _texturePrefix) ?
            name : _otherMaterialsPath + "/" + name;

        populator.addPath(texturePath, [&](wxutil::TreeModel::Row& row,
            const std::string& path, const std::string& leafName, bool isFolder)
        {
            row[_columns.isOtherMaterialsFolder] = false;
            AssignValuesToRow(row, path, isFolder ? path : name, leafName, isFolder);
        });
    });
}

wxutil::TreeModel::Row MaterialPopulator::InsertFolder(const wxutil::TreeModel::Ptr& model, 
    const std::string& path, const std::string& leafName, const wxDataViewItem& parentItem, bool isOtherMaterial)
{
    // Append a node to the tree view for this child
    auto row = model->AddItem(parentItem);

    row[_columns.isOtherMaterialsFolder] = isOtherMaterial;

    AssignValuesToRow(row, path, path, leafName, true);

    return row;
}

void MaterialPopulator::InsertTexture(const wxutil::TreeModel::Ptr& model,
    const std::string& path, const std::string& declName, const std::string& leafName, const wxDataViewItem& parentItem)
{
    auto row = model->AddItem(parentItem);

    // Assign additional columns
    row[_columns.isOtherMaterialsFolder] = false;

    // Base class call will invoke Row::SendItemAdded()
    AssignValuesToRow(row, path, declName, leafName, false);
}

void MaterialPopulator::SortModel(const wxutil::TreeModel::Ptr& model)
{
    // Sort the model while we're still in the worker thread
    SortModel(model, wxDataViewItem());
}

void MaterialPopulator::SortModel(const wxutil::TreeModel::Ptr& model, const wxDataViewItem& startItem)
{
    model->SortModelFoldersFirst(startItem, _columns.iconAndName, _columns.isFolder,
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
