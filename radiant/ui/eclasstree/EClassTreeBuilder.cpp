#include "EClassTreeBuilder.h"

#include "EClassTree.h"
#include "ifavourites.h"

#include "wxutil/Bitmap.h"

#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

namespace ui
{

namespace
{
	constexpr const char* ENTITY_ICON = "cmenu_add_entity.png";
}

EClassTreeBuilder::EClassTreeBuilder(const wxutil::DeclarationTreeView::Columns& columns) :
    ThreadedDeclarationTreePopulator(columns),
    _columns(columns)
{
    // Get the list of favourites
    _favourites = GlobalFavouritesManager().getFavourites(decl::getTypeName(decl::Type::EntityDef));

    _entityIcon.CopyFromBitmap(wxutil::GetLocalBitmap(ENTITY_ICON));
}

EClassTreeBuilder::~EClassTreeBuilder()
{
    EnsureStopped();
}

void EClassTreeBuilder::PopulateModel(const wxutil::TreeModel::Ptr& model)
{
    _treePopulator = std::make_unique<wxutil::VFSTreePopulator>(model);

    ThrowIfCancellationRequested();

    GlobalEntityClassManager().forEachEntityClass(*this);

    ThrowIfCancellationRequested();
}

void EClassTreeBuilder::SortModel(const wxutil::TreeModel::Ptr& model)
{
    model->SortModelByColumn(_columns.leafName);
}

void EClassTreeBuilder::visit(const IEntityClassPtr& eclass)
{
    ThrowIfCancellationRequested();

	// Prefix mod name
	std::string fullPath = eclass->getModName() + "/";

	// Prefix inheritance path (recursively)
	fullPath += GetInheritancePathRecursively(*eclass);

	// The entityDef name itself
	fullPath += eclass->getDeclName();

	// Let the VFSTreePopulator sort this into the tree
    _treePopulator->addPath(fullPath, [&](wxutil::TreeModel::Row& row,
        const std::string& path, const std::string& leafName, bool isFolder)
        {
            bool isFavourite = _favourites.count(leafName) > 0;

            row[_columns.iconAndName] = wxVariant(wxDataViewIconText(leafName, _entityIcon));
            row[_columns.iconAndName] = wxutil::TreeViewItemStyle::Declaration(isFavourite);
            row[_columns.fullName] = leafName;
            row[_columns.leafName] = leafName;
            row[_columns.declName] = leafName;
            row[_columns.isFolder] = false;
            row[_columns.isFavourite] = isFavourite;

            row.SendItemAdded();
        });
}

std::string EClassTreeBuilder::GetInheritancePathRecursively(IEntityClass& eclass)
{
    std::string returnValue;

    if (auto parent = eclass.getParent(); parent)
    {
        returnValue += GetInheritancePathRecursively(*parent);
        returnValue += parent->getDeclName() + "/";
    }

    return returnValue;
}

} // namespace ui

