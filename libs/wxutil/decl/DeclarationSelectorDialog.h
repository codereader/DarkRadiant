#pragma once

#include <wx/dataview.h>

#include "ui/iwindowstate.h"
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
    public DialogBase,
    public ui::IPersistableObject
{
private:
    decl::Type _declType;

    DeclarationSelector* _selector;
    wxSizer* _mainSizer;
    wxStdDialogButtonSizer* _buttonSizer;

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

    wxButton* GetAffirmativeButton();

private:
    void HandleTreeViewSelectionChanged();
    void onDeclSelectionChanged(wxDataViewEvent& ev);
    void onDeclItemActivated(wxDataViewEvent& ev);
};

}
