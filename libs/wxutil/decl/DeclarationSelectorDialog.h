#pragma once

#include <wx/dataview.h>

#include "idecltypes.h"
#include "../dialog/DialogBase.h"

class wxSizer;
class wxStdDialogButtonSizer;

namespace wxutil
{

class DeclarationSelector;

/**
 * Base implementation of a Declaration chooser dialog.
 *
 * Provides a tree view of available declaration items plus optional previews
 * of the active selection.
 */
class DeclarationSelectorDialog :
    public DialogBase
{
private:
    decl::Type _declType;

    DeclarationSelector* _selector;
    wxSizer* _mainSizer;
    wxStdDialogButtonSizer* _buttonSizer;

public:
    DeclarationSelectorDialog(decl::Type declType, const std::string& title, 
        const std::string& windowName, wxWindow* parent = nullptr);

    // Get the currently selected declaration name
    virtual std::string GetSelectedDeclName();

    // Set the declaration selection in the selector
    virtual void SetSelectedDeclName(const std::string& declName);

    int ShowModal() override;

protected:
    void SetSelector(DeclarationSelector* selector);

    wxButton* GetAffirmativeButton();

private:
    void HandleTreeViewSelectionChanged();
    void onTreeViewSelectionChanged(wxDataViewEvent& ev);
};

}
