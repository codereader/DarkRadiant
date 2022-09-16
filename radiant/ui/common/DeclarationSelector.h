#pragma once

#include "idecltypes.h"
#include <wx/panel.h>

#include "wxutil/dataview/DeclarationTreeView.h"

namespace ui
{

/**
 * Common implementation of a declaration selector widget that can be added to a dialog.
 */
class DeclarationSelector :
    public wxPanel
{
private:
    decl::Type _declType;

    const wxutil::DeclarationTreeView::Columns& _columns;
    wxutil::DeclarationTreeView* _treeView;

public:
    DeclarationSelector(wxWindow* parent, decl::Type declType, const wxutil::DeclarationTreeView::Columns& columns);

protected:
    wxutil::DeclarationTreeView* GetTreeView();
    void PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator);

private:
    void createTreeView();
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
};

}
