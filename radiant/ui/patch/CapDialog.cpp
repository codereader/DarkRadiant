#include "CapDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>

#include "selectionlib.h"
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

namespace
{
	const char* WINDOW_TITLE = N_("Create Cap Patch");

	const char* const CAPTYPE_NAMES[eNumCapTypes] =
	{
		N_("Bevel"),
		N_("End Cap"),
		N_("Inverted Bevel"),
		N_("Inverted Endcap"),
		N_("Cylinder"),
	};
}

PatchCapDialog::PatchCapDialog() :
	Dialog(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow())
{
	// 5x2 table for the radiobuttons + images
	wxFlexGridSizer* sizer = new wxFlexGridSizer(3, 4, 12, 6);

	sizer->AddGrowableCol(1);
	sizer->AddGrowableCol(3);

	_dialog->GetSizer()->Add(sizer, 0, wxEXPAND | wxALL, 12);

	addItemToTable(sizer, "cap_bevel.png", patch::CapType::Bevel);
	addItemToTable(sizer, "cap_ibevel.png", patch::CapType::InvertedBevel);
	addItemToTable(sizer, "cap_endcap.png", patch::CapType::EndCap);
	addItemToTable(sizer, "cap_iendcap.png", patch::CapType::InvertedEndCap);
	addItemToTable(sizer, "cap_cylinder.png", patch::CapType::Cylinder);
}

void PatchCapDialog::addItemToTable(wxFlexGridSizer* sizer, const std::string& image, patch::CapType type)
{
	wxStaticBitmap* img = new wxStaticBitmap(_dialog, wxID_ANY, 
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + image));

	wxRadioButton* radioButton = new wxRadioButton(_dialog, wxID_ANY, _(CAPTYPE_NAMES[type]), 
		wxDefaultPosition, wxDefaultSize, type == patch::CapType::Bevel ? wxRB_GROUP : 0);

	sizer->Add(img, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	sizer->Add(radioButton, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
	
	// Store the widget in the local map
	_radioButtons[type] = radioButton;
}

patch::CapType PatchCapDialog::getSelectedCapType()
{
	if (_result != RESULT_OK) return patch::CapType::None;

	for (const auto& pair : _radioButtons)
	{
		if (pair.second->GetValue())
		{
			return pair.first;
		}
	}

	return patch::CapType::None; // invalid
}

void PatchCapDialog::Show(const cmd::ArgumentList & args)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		wxutil::Messagebox::ShowError(_("Cannot create caps, no patches selected."));
		return;
	}

	PatchCapDialog dialog;

	if (dialog.run() == IDialog::RESULT_OK)
	{
		GlobalCommandSystem().execute("CapSelectedPatches");
	}
}

} // namespace ui
