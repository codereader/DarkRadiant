#pragma once

#include "ieclass.h"

#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

#include <wx/icon.h>

namespace wxutil { class VFSTreePopulator; }

namespace ui
{

/**
 * Visitor class to retrieve entityDef names and sort them into the hierarchy tree.
 */
class EClassTreeBuilder final :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
	const wxutil::DeclarationTreeView::Columns& _columns;

	wxIcon _entityIcon;

    std::unique_ptr<wxutil::VFSTreePopulator> _treePopulator;

public:
    EClassTreeBuilder(const wxutil::DeclarationTreeView::Columns& columns);

    ~EClassTreeBuilder();

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override;
    void SortModel(const wxutil::TreeModel::Ptr& model) override;

private:
    // Returns an inheritance path, like this: "moveables/swords/"
    static std::string GetInheritancePathRecursively(IEntityClass& eclass);
};

} // namespace
