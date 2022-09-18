#include "DeclarationSelectorDialog.h"

#include "DeclarationSelector.h"
#include <wx/sizer.h>

namespace wxutil
{

DeclarationSelectorDialog::DeclarationSelectorDialog(decl::Type declType, 
    const std::string& title, const std::string& windowName, wxWindow* parent) :
    DialogBase(title, parent, windowName),
    _declType(declType)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Inner sizer with 12-pixel padding
    _mainSizer = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(_mainSizer, 1, wxEXPAND | wxALL, 12);

    // Button row
    _buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    _mainSizer->Add(_buttonSizer, 0, wxALIGN_RIGHT, 12);
}

void DeclarationSelectorDialog::SetSelector(DeclarationSelector* selector)
{
    _selector = selector;
    _selector->Reparent(this);

    _mainSizer->Prepend(_selector, 1, wxEXPAND | wxBOTTOM, 12);

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
}

wxButton* DeclarationSelectorDialog::GetAffirmativeButton()
{
    return _buttonSizer->GetAffirmativeButton();
}

}
