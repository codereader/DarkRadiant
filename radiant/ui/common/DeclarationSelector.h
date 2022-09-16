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
    // Construct a selector widget with the default set of tree view columns
    DeclarationSelector(wxWindow* parent, decl::Type declType);

    // Construct a selector widget with the given set of tree view columns
    DeclarationSelector(wxWindow* parent, decl::Type declType, const wxutil::DeclarationTreeView::Columns& columns);

    /**
     * Return the declaration selected by the user, or an empty string if there
     * was no selection.
     */
    virtual std::string GetSelectedDeclName() const;

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
    wxutil::DeclarationTreeView* GetTreeView() const;
    const wxutil::DeclarationTreeView::Columns& GetColumns() const;

    void PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator);

    // Event method invoked when the tree view selection has been changed
    virtual void onTreeViewSelectionChanged()
    {}

    // Default tree view columns. Subclasses can use a different set of columns if needed
    static const wxutil::DeclarationTreeView::Columns& CreateDefaultColumns();

private:
    void createTreeView();
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
};

}
