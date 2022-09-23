#include "ParticleChooserDialog.h"

#include "i18n.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/radiobut.h>

namespace ui
{

ParticleChooserDialog::ParticleChooserDialog(bool showClassnameSelector) :
    DeclarationSelectorDialog(decl::Type::Particle, _("Choose Particle"), "ParticleChooser"),
    _selector(new ParticleSelector(this)),
    _funcEmitter(nullptr),
    _funcSmoke(nullptr)
{
    SetSelector(_selector);

    if (showClassnameSelector)
    {
        auto radioHbox = new wxBoxSizer(wxHORIZONTAL);

        auto classText = new wxStaticText(this, wxID_ANY, _("Entity Class to create:"));

        _funcEmitter = new wxRadioButton(this, wxID_ANY, "func_emitter", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        _funcSmoke = new wxRadioButton(this, wxID_ANY, "func_smoke");

        _funcEmitter->SetValue(true);
        
        radioHbox->Add(classText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 0);
        radioHbox->Add(_funcEmitter, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        radioHbox->Add(_funcSmoke, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

        AddItemToBottomRow(radioHbox);
    }
}

std::string ParticleChooserDialog::getSelectedClassname()
{
    if (_funcSmoke == nullptr || _funcEmitter == nullptr) return "";

    return _funcEmitter->GetValue() ? "func_emitter" : "func_smoke";
}

std::string ParticleChooserDialog::ChooseParticle(const std::string& currentParticle)
{
    return RunDialog(false, currentParticle).selectedParticle;
}

ParticleChooserDialog::SelectionResult ParticleChooserDialog::ChooseParticleAndEmitter(const std::string& currentParticle)
{
    return RunDialog(true, currentParticle);
}

ParticleChooserDialog::SelectionResult ParticleChooserDialog::RunDialog(bool showClassnameSelector, const std::string& currentParticle)
{
    auto* dialog = new ParticleChooserDialog(showClassnameSelector);

    // Prefer SetSelectedParticle to handle the .prt extension
    dialog->_selector->SetSelectedParticle(currentParticle);

    auto result = dialog->ShowModal();

    SelectionResult selectionResult;

    if (result == wxID_OK)
    {
        // GetSelectedParticle will return the name including the .prt ending
        selectionResult.selectedParticle = result == wxID_OK ? dialog->_selector->GetSelectedParticle() : "";
        selectionResult.selectedClassname = showClassnameSelector ? dialog->getSelectedClassname() : "";
    }

    dialog->Destroy();

    return selectionResult;
}

} // namespace
