#pragma once

#include <vector>
#include <sigc++/connection.h>

#include "idecltypes.h"
#include "ui/iwindowstate.h"
#include <wx/panel.h>
#include <wx/sizer.h>

#include "ui/ideclpreview.h"
#include "../dataview/DeclarationTreeView.h"
#include "wxutil/PanedPosition.h"

namespace wxutil
{

class DeclFileInfo;

/**
 * Common implementation of a declaration selector widget that can be added to a dialog.
 * It features a Declaration Tree View with an associated toolbar for favourite management.
 * Preview widgets can optionally be appended to the right or the bottom of the tree view.
 */
class DeclarationSelector :
    public wxPanel,
    public ui::IPersistableObject
{
private:
    decl::Type _declType;

    const DeclarationTreeView::Columns& _columns;
    DeclarationTreeView* _treeView;

    wxPanel* _leftPanel;
    wxPanel* _rightPanel;

    wxSizer* _treeVbox;

    // The set of preview widgets attached to this selector
    std::vector<ui::IDeclarationPreview*> _previews;

    DeclFileInfo* _declFileInfo;

    std::unique_ptr<PanedPosition> _panedPosition;

    sigc::connection _declsReloaded;

public:
    // Construct a selector widget with the default set of tree view columns
    DeclarationSelector(wxWindow* parent, decl::Type declType);

    // Construct a selector widget with the given set of tree view columns
    DeclarationSelector(wxWindow* parent, decl::Type declType, const DeclarationTreeView::Columns& columns);

    ~DeclarationSelector() override;

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

    // Set the focus on the treeview widget
    void FocusTreeView();

    void loadFromPath(const std::string& registryKey) override;
    void saveToPath(const std::string& registryKey) override;

    // (Re-)populate the tree view, must be implemented by subclasses
    virtual void Populate() = 0;

protected:
    DeclarationTreeView* GetTreeView() const;

    // Adds a preview widget to the right of the tree view
    void AddPreviewToRightPane(ui::IDeclarationPreview* preview, int sizerProportion = 1);
    void AddPreviewToBottom(ui::IDeclarationPreview* preview, int sizerProportion = 0);

    void AddWidgetToBottom(wxWindow* widget, int sizerProportion = 0);

    const DeclarationTreeView::Columns& GetColumns() const;

    void PopulateTreeView(const IResourceTreePopulator::Ptr& populator);

    // Event method invoked when the tree view selection has been changed
    virtual void onTreeViewSelectionChanged()
    {}

    // Event method invoked when the tree view selection has been activated (Enter key or double-click)
    // Returns true to indicate that the event has been handled and should not be further processed
    virtual bool onTreeViewItemActivated()
    {
        return false;
    }

    // Default tree view columns. Subclasses can use a different set of columns if needed
    static const DeclarationTreeView::Columns& CreateDefaultColumns();

private:
    void createTreeView(wxWindow* parent);
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
    void onTreeViewItemActivated(wxDataViewEvent& ev);
};

}
