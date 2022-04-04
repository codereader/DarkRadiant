#include "ParticleChooserDialog.h"

#include "i18n.h"

#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/radiobut.h>

namespace ui
{

ParticleChooserDialog::ParticleChooserDialog(bool showClassnameSelector) :
	DialogBase(_("Choose Particle")),
    _selector(new ParticleSelector(this)),
    _funcEmitter(nullptr),
    _funcSmoke(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	GetSizer()->Add(_selector, 1, wxEXPAND | wxALL, 12);

    auto bottomHbox = new wxBoxSizer(wxHORIZONTAL);

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

        bottomHbox->Add(radioHbox, 0, wxLEFT, 12);
    }

    bottomHbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 1, wxALIGN_CENTER_VERTICAL);

	GetSizer()->Add(bottomHbox, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.6f);

    _selector->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ParticleChooserDialog::_onItemActivated, this);
}

std::string ParticleChooserDialog::getSelectedClassname()
{
    if (_funcSmoke == nullptr || _funcEmitter == nullptr) return "";

    return _funcEmitter->GetValue() ? "func_emitter" : "func_smoke";
}

void ParticleChooserDialog::_onItemActivated(wxDataViewEvent& ev)
{
    EndModal(wxID_OK);
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

    dialog->_selector->setSelectedParticle(currentParticle);

    auto result = dialog->ShowModal();

    SelectionResult selectionResult;

    if (result == wxID_OK)
    {
        selectionResult.selectedParticle = result == wxID_OK ? dialog->_selector->getSelectedParticle() : "";
        selectionResult.selectedClassname = showClassnameSelector ? dialog->getSelectedClassname() : "";
    }

    dialog->Destroy();

    return selectionResult;
}

} // namespace
