#include "ParticleChooserDialog.h"

#include "i18n.h"

#include "wxutil/dataview/ThreadedResourceTreePopulator.h"
#include "wxutil/dataview/TreeViewItemStyle.h"

#include <wx/sizer.h>

namespace ui
{

ParticleChooserDialog::ParticleChooserDialog() :
	DialogBase(_("Choose Particle")),
    _selector(new ParticleSelector(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	
	GetSizer()->Add(_selector, 1, wxEXPAND | wxALL, 12);
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);

	FitToScreen(0.5f, 0.6f);

    _selector->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &ParticleChooserDialog::_onItemActivated, this);
}

void ParticleChooserDialog::_onItemActivated(wxDataViewEvent& ev)
{
    EndModal(wxID_OK);
}

std::string ParticleChooserDialog::ChooseParticle(const std::string& current)
{
    auto* dialog = new ParticleChooserDialog();
	
	dialog->_selector->setSelectedParticle(current);

	auto returnValue = dialog->ShowModal() == wxID_OK ? dialog->_selector->getSelectedParticle() : "";

    dialog->Destroy();

    return returnValue;
}

} // namespace
