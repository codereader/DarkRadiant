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

    /**
     * Return the declaration selected by the user, or an empty string if there
     * was no selection.
     */
    virtual std::string GetSelectedDeclName();

    /**
     * Set the given declaration name as the current selection, highlighting it
     * in the tree view.
     *
     * @param declName
     * The fullname of the declaration to select, or the empty string if there
     * should be no selection.
     */
    virtual void SetSelectedDeclName(const std::string& declName);

protected:
    wxutil::DeclarationTreeView* GetTreeView();
    void PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator);

private:
    void createTreeView();
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
};

}
