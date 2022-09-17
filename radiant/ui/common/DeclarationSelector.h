#pragma once

#include "idecltypes.h"
#include <wx/panel.h>

#include "wxutil/dataview/DeclarationTreeView.h"

namespace ui
{

/**
 * Common implementation of a declaration selector widget that can be added to a dialog.
 * It features a Declaration Tree View with an associated toolbar for favourite management.
 * Preview widgets can optionally be appended to the right or the bottom of the tree view.
 */
class DeclarationSelector :
    public wxPanel
{
private:
    decl::Type _declType;

    const wxutil::DeclarationTreeView::Columns& _columns;
    wxutil::DeclarationTreeView* _treeView;

    wxSizer* _horizontalSizer;
    wxSizerItem* _treeViewSizerItem;

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

    // Adds a preview widget to the right of the tree view
    void AddPreviewToRightPane(wxWindow* preview, int sizerProportion = 1);
    void AddPreviewToBottom(wxWindow* preview, int sizerProportion = 0);

    const wxutil::DeclarationTreeView::Columns& GetColumns() const;

    void PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator);

    // Event method invoked when the tree view selection has been changed
    virtual void onTreeViewSelectionChanged()
    {}

    // Event method invoked when the tree view selection has been activated (Enter key or double-click)
    virtual void onTreeViewItemActivated()
    {}

    // Default tree view columns. Subclasses can use a different set of columns if needed
    static const wxutil::DeclarationTreeView::Columns& CreateDefaultColumns();

private:
    void createTreeView();
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
    void onTreeViewItemActivated(wxDataViewEvent& ev);
};

}
