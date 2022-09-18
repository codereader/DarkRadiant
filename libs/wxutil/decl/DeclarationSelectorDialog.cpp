#include "DeclarationSelectorDialog.h"

#include "DeclarationSelector.h"
#include <wx/button.h>
#include <wx/sizer.h>

#include "iregistry.h"

namespace wxutil
{

DeclarationSelectorDialog::DeclarationSelectorDialog(decl::Type declType, 
    const std::string& title, const std::string& windowName, wxWindow* parent) :
    DialogBase(title, parent, windowName),
    _declType(declType),
    _selector(nullptr),
    _mainSizer(nullptr),
    _buttonSizer(nullptr),
    _restoreSelectionFromRegistry(true)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Inner sizer with 12-pixel padding
    _mainSizer = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(_mainSizer, 1, wxEXPAND | wxALL, 12);

    // Button row
    _buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    _mainSizer->Add(_buttonSizer, 0, wxALIGN_RIGHT, 12);

    // Save the state of this dialog on close
    RegisterPersistableObject(this);
}

void DeclarationSelectorDialog::SetSelector(DeclarationSelector* selector)
{
    if (_selector != nullptr)
    {
        throw std::logic_error("There's already a selector attached to this dialog");
    }

    _selector = selector;
    _selector->Reparent(this);

    _mainSizer->Prepend(_selector, 1, wxEXPAND | wxBOTTOM, 12);

    // Update the affirmative button's sensitivity based on the selection
    _selector->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DeclarationSelectorDialog::onDeclSelectionChanged, this);
    _selector->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DeclarationSelectorDialog::onDeclItemActivated, this);

    // The selector state should be persisted on dialog close
    RegisterPersistableObject(_selector);
}

std::string DeclarationSelectorDialog::GetSelectedDeclName()
{
    return _selector->GetSelectedDeclName();
}

void DeclarationSelectorDialog::SetSelectedDeclName(const std::string& declName)
{
    _selector->SetSelectedDeclName(declName);
    _restoreSelectionFromRegistry = false; // prevent this selection from being overwrittenvoid
}

int DeclarationSelectorDialog::ShowModal()
{
    if (_selector == nullptr)
    {
        throw std::logic_error("Cannot run the DeclarationSelectorDialog without selector");
    }

    HandleTreeViewSelectionChanged();

    _selector->FocusTreeView();

    return DialogBase::ShowModal();
}

wxButton* DeclarationSelectorDialog::GetAffirmativeButton()
{
    return _buttonSizer->GetAffirmativeButton();
}

void DeclarationSelectorDialog::HandleTreeViewSelectionChanged()
{
    GetAffirmativeButton()->Enable(!_selector->GetSelectedDeclName().empty());
}

void DeclarationSelectorDialog::onDeclSelectionChanged(wxDataViewEvent&)
{
    HandleTreeViewSelectionChanged();
}

void DeclarationSelectorDialog::onDeclItemActivated(wxDataViewEvent&)
{
    // Double-clicking a valid decl item positively closes the dialog
    if (!_selector->GetSelectedDeclName().empty())
    {
        EndModal(wxID_OK);
    }
}

void DeclarationSelectorDialog::loadFromPath(const std::string& registryKey)
{
    if (!_restoreSelectionFromRegistry) return;

    auto lastSelectedDeclName = GlobalRegistry().getAttribute(registryKey, "lastSelectedDeclName");

    if (!lastSelectedDeclName.empty())
    {
        SetSelectedDeclName(lastSelectedDeclName);
    }
}

void DeclarationSelectorDialog::saveToPath(const std::string& registryKey)
{
    GlobalRegistry().setAttribute(registryKey, "lastSelectedDeclName", GetSelectedDeclName());
}

}
