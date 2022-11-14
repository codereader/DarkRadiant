#pragma once

#include "idecltypes.h"
#include "ifavourites.h"

#include "../Bitmap.h"
#include "../Icon.h"
#include "DeclarationTreeView.h"
#include "ThreadedResourceTreePopulator.h"
#include "TreeViewItemStyle.h"
#include "VFSTreePopulator.h"
#include "os/path.h"
#include "string/replace.h"
#include "string/split.h"

namespace wxutil
{

/**
 * Shared ThreadedResourceTreePopulator implementation specialising on populating
 * trees of IDeclaration elements.
 */
class ThreadedDeclarationTreePopulator :
    public ThreadedResourceTreePopulator
{
private:
    static constexpr const char* const DEFAULT_DECL_ICON = "decl.png";
    static constexpr const char* const DEFAULT_FOLDER_ICON = "folder16.png";

    decl::Type _type;
    const DeclarationTreeView::Columns& _columns;

    std::set<std::string> _favourites;

    Icon _folderIcon;
    Icon _declIcon;

public:
    ThreadedDeclarationTreePopulator(decl::Type type, const DeclarationTreeView::Columns& columns) :
        ThreadedDeclarationTreePopulator(type, columns, DEFAULT_DECL_ICON, DEFAULT_FOLDER_ICON)
    {}

    ThreadedDeclarationTreePopulator(decl::Type type, const DeclarationTreeView::Columns& columns, 
        const std::string& declIcon) :
        ThreadedDeclarationTreePopulator(type, columns, declIcon, DEFAULT_FOLDER_ICON)
    {}

    ThreadedDeclarationTreePopulator(decl::Type type, const DeclarationTreeView::Columns& columns, 
        const std::string& declIcon, const std::string& folderIcon) :
        ThreadedResourceTreePopulator(columns),
        _type(type),
        _columns(columns),
        _declIcon(GetLocalBitmap(declIcon)),
        _folderIcon(GetLocalBitmap(folderIcon))
    {
        // Assemble the set of favourites for the given declaration type
        _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(type));
    }

    ~ThreadedDeclarationTreePopulator() override
    {
        EnsureStopped();
    }

    // Default implementation creates a plain tree using the mod name as first path element
    // Subclasses should override the default implementation (without calling the base) if not suitable
    void PopulateModel(const TreeModel::Ptr& model) override
    {
        VFSTreePopulator populator(model);

        GlobalDeclarationManager().foreachDeclaration(_type, [&](const decl::IDeclaration::Ptr& decl)
        {
            ThrowIfCancellationRequested();

            auto fullPath = GenerateFullDeclPath(decl);

            // Sort the decl into the tree and set the values
            populator.addPath(fullPath, [&](TreeModel::Row& row,
                const std::string& path, const std::string& leafName, bool isFolder)
            {
                AssignValuesToRow(row, path, isFolder ? path : decl->getDeclName(), leafName, isFolder);
            });
        });
    }

    // Generates the full path the given declaration should be sorted into.
    static std::string GenerateFullDeclPath(const decl::IDeclaration::Ptr& decl)
    {
        // Some names contain backslashes, sort them in the tree by replacing the backslashes
        auto nameForwardSlashes = os::standardPath(decl->getDeclName());

        return decl->getModName() + "/" + nameForwardSlashes;
    }

    // Add the given named decl to the tree (assuming it was not present before)
    void AddSingleDecl(const TreeModel::Ptr& model, const std::string& declName)
    {
        auto decl = GlobalDeclarationManager().findDeclaration(_type, declName);

        if (!decl) return;

        std::vector<std::string> parts;
        string::split(parts, GenerateFullDeclPath(decl), "/");

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
                auto row = InsertFolder(model, parentPath, parts[i], parentItem);

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
            InsertDecl(model, itemPath, declName, parts.back(), parentItem);

            // Sort the subtree starting from this parent item
            SortModel(model, parentItem);

            // Force a reload of this subtree by sending events for each child
            model->SendSubtreeRefreshEvents(parentItem);
        }
    }

    // Remove the given named decl from the tree (assuming it is present)
    void RemoveSingleDecl(const TreeModel::Ptr& model, const std::string& declName)
    {
        auto normalisedName = string::replace_all_copy(declName, "//", "/");

        // Walk up the parents to check if the removed material leaves any empty folders behind
        auto item = model->FindString(normalisedName, _columns.declName);

        // Prevent removal of items with children
        wxDataViewItemArray children;
        while (item.IsOk() && model->GetChildren(item, children) == 0)
        {
            auto parentItem = model->GetParent(item);

            model->RemoveItem(item);

            if (!parentItem.IsOk()) break;

            item = parentItem; // remove the parent too (if it is empty)
        }
    }

protected:
    // Default sorting behaviour is to sort the tree alphabetically with folders on top
    void SortModel(const TreeModel::Ptr& model) override
    {
        SortModel(model, wxDataViewItem());
    }

    void SortModel(const TreeModel::Ptr& model, const wxDataViewItem& startItem)
    {
        model->SortModelFoldersFirst(startItem, _columns.leafName, _columns.isFolder);
    }

    /**
     * Populates the given row with values matching for a certain declaration or folder
     *
     * @fullPath: The path to the row, mainly used for internal storage.
     * @declName: The name of the declaration (including any folders and slashes)
     * @leafName: The name part after the rightmost slash
     * @isFolder: Whether this row is a folder (the folder icon will be assigned)
     */
    void AssignValuesToRow(TreeModel::Row& row, const std::string& fullPath, 
        const std::string& declName, const std::string& leafName, bool isFolder)
    {
        ThrowIfCancellationRequested();

        auto isFavourite = IsFavourite(declName);

        row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, !isFolder ? _declIcon : _folderIcon));
        row[_columns.iconAndName].setAttr(TreeViewItemStyle::Declaration(isFavourite));
        row[_columns.fullName] = fullPath;
        row[_columns.leafName] = leafName;
        row[_columns.declName] = declName;
        row[_columns.isFolder] = isFolder;
        row[_columns.isFavourite] = isFavourite;

        row.SendItemAdded();
    }

    const std::set<std::string>& GetFavourites() const
    {
        return _favourites;
    }

    bool IsFavourite(const std::string& declName)
    {
        return _favourites.count(declName) > 0;
    }

    TreeModel::Row InsertFolder(const TreeModel::Ptr& model, const std::string& path, 
        const std::string& leafName, const wxDataViewItem& parentItem)
    {
        // Append a node to the tree view for this child
        auto row = model->AddItem(parentItem);

        AssignValuesToRow(row, path, path, leafName, true);

        return row;
    }

    void InsertDecl(const TreeModel::Ptr& model, const std::string& path, const std::string& declName, 
        const std::string& leafName, const wxDataViewItem& parentItem)
    {
        auto row = model->AddItem(parentItem);

        // Call will invoke Row::SendItemAdded()
        AssignValuesToRow(row, path, declName, leafName, false);
    }
};

}
