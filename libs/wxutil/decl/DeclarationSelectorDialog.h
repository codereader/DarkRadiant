#pragma once

#include <wx/dataview.h>

#include "ui/iwindowstate.h"
#include "idecltypes.h"
#include "../dialog/DialogBase.h"

class wxSizer;
class wxButton;
class wxStdDialogButtonSizer;

namespace wxutil
{

class DeclarationSelector;

/**
 * Base implementation of a Declaration chooser dialog.
 *
 * Provides a tree view of available declaration items plus optional previews
 * of the active selection.
 *
 * It will attempt to restore the last selected item from the registry when shown,
 * unless the SetSelectedDeclName() method is called, which always takes precedence.
 */
class DeclarationSelectorDialog :
    public DialogBase,
    public ui::IPersistableObject
{
private:
    decl::Type _declType;

    DeclarationSelector* _selector;
    wxSizer* _mainSizer;
    wxSizer* _bottomRowSizer;
    wxStdDialogButtonSizer* _buttonSizer;
    wxButton* _reloadDeclsButton;

    bool _restoreSelectionFromRegistry;

public:
    DeclarationSelectorDialog(decl::Type declType, const std::string& title, 
        const std::string& windowName, wxWindow* parent = nullptr);

    // Get the currently selected declaration name
    virtual std::string GetSelectedDeclName();

    // Set the declaration selection in the selector
    virtual void SetSelectedDeclName(const std::string& declName);

    int ShowModal() override;

    void loadFromPath(const std::string& registryKey) override;
    void saveToPath(const std::string& registryKey) override;

protected:
    void SetSelector(DeclarationSelector* selector);

    // Adds a widget to the bottom row, to the left of the standard buttons
    void AddItemToBottomRow(wxWindow* widget);
    // Adds a widget to the bottom row, to the left of the standard buttons
    void AddItemToBottomRow(wxSizer* sizer);

    wxButton* GetAffirmativeButton();

private:
    void HandleTreeViewSelectionChanged();
    void onDeclSelectionChanged(wxDataViewEvent& ev);
    void onDeclItemActivated(wxDataViewEvent& ev);
    void onReloadDecls(wxCommandEvent& ev);
};

}
