#include "EClassTreeBuilder.h"

#include "EClassTree.h"

#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeModel.h"

namespace ui
{

namespace
{
	constexpr const char* ENTITY_ICON = "cmenu_add_entity.png";
}

EClassTreeBuilder::EClassTreeBuilder(const wxutil::DeclarationTreeView::Columns& columns) :
    ThreadedDeclarationTreePopulator(decl::Type::EntityDef, columns, ENTITY_ICON)
{}

EClassTreeBuilder::~EClassTreeBuilder()
{
    EnsureStopped();
}

void EClassTreeBuilder::PopulateModel(const wxutil::TreeModel::Ptr& model)
{
    _treePopulator = std::make_unique<wxutil::VFSTreePopulator>(model);

    ThrowIfCancellationRequested();

    GlobalEntityClassManager().forEachEntityClass([&](const IEntityClassPtr& eclass)
    {
        ThrowIfCancellationRequested();

        // Prefix mod name
        auto fullPath = eclass->getModName() + "/";

        // Prefix inheritance path (recursively)
        fullPath += GetInheritancePathRecursively(*eclass);

        // The entityDef name itself
        fullPath += eclass->getDeclName();

        // Let the VFSTreePopulator sort this into the tree
        _treePopulator->addPath(fullPath, [&](wxutil::TreeModel::Row& row,
            const std::string& path, const std::string& leafName, bool isFolder)
        {
            AssignValuesToRow(row, path, leafName, leafName, false);
        });
    });

    ThrowIfCancellationRequested();
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

