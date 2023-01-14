#pragma once

#include "wxutil/decl/DeclarationSelectorDialog.h"

namespace ui
{

/**
 * Dialog for listing and selection of FX declarations.
 */
class FxChooser :
    public wxutil::DeclarationSelectorDialog
{
private:
    FxChooser(wxWindow* parent = nullptr);

public:
    // Run the dialog and return the selected decl - this will be empty if the user clicks cancel
    static std::string ChooseDeclaration(const std::string& preselected = std::string());
};

}
