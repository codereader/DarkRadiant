#include "DeclarationSelectorDialog.h"

#include "DeclarationSelector.h"
#include <wx/button.h>
#include <wx/sizer.h>

#include "i18n.h"
#include "ideclmanager.h"
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

    // Bottom row
    auto grid = new wxFlexGridSizer(1, 2, 0, 12);
    grid->AddGrowableCol(0);
    grid->AddGrowableCol(1);

    // Left half
    _bottomRowSizer = new wxBoxSizer(wxHORIZONTAL);
    grid->Add(_bottomRowSizer, 1, wxALIGN_LEFT);

    // Right half contains the buttons
    _buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);

    // Add a Reload Decls button
    _reloadDeclsButton = new wxButton(this, wxID_ANY, _("Reload Decls"));
    _reloadDeclsButton->Bind(wxEVT_BUTTON, &DeclarationSelectorDialog::onReloadDecls, this);
    _buttonSizer->Prepend(_reloadDeclsButton, 0, wxLEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    grid->Add(_buttonSizer, 0, wxALIGN_RIGHT);

    _mainSizer->Add(grid, 0, wxEXPAND, 12);

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

void DeclarationSelectorDialog::AddItemToBottomRow(wxWindow* widget)
{
    _bottomRowSizer->Prepend(widget, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
}

void DeclarationSelectorDialog::AddItemToBottomRow(wxSizer* sizer)
{
    _bottomRowSizer->Prepend(sizer, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
}

std::string DeclarationSelectorDialog::GetSelectedDeclName()
{
    return _selector->GetSelectedDeclName();
}

void DeclarationSelectorDialog::SetSelectedDeclName(const std::string& declName)
{
    _selector->SetSelectedDeclName(declName);
    _restoreSelectionFromRegistry = false; // prevent this selection from being overwritten
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

void DeclarationSelectorDialog::onReloadDecls(wxCommandEvent& ev)
{
    GlobalDeclarationManager().reloadDeclarations();
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
