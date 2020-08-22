#include "CapDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>

#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"
#include "command/ExecutionFailure.h"
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

namespace
{
	const char* WINDOW_TITLE = N_("Create Cap Patch");

	inline std::string getCapTypeName(patch::CapType capType)
	{
		switch (capType)
		{
		case patch::CapType::Bevel: return _("Bevel");
		case patch::CapType::EndCap: return _("End Cap");
		case patch::CapType::InvertedBevel: return _("Inverted Bevel");
		case patch::CapType::InvertedEndCap: return _("Inverted Endcap");
		case patch::CapType::Cylinder: return _("Cylinder");
		default: return "";
		};
	}
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

	wxRadioButton* radioButton = new wxRadioButton(_dialog, wxID_ANY, getCapTypeName(type),
		wxDefaultPosition, wxDefaultSize, type == patch::CapType::Bevel ? wxRB_GROUP : 0);

	sizer->Add(img, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL);
	sizer->Add(radioButton, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
	
	// Store the widget in the local map
	_radioButtons[type] = radioButton;
}

patch::CapType PatchCapDialog::getSelectedCapType()
{
	if (_result != RESULT_OK) return patch::CapType::Nocap;

	for (const auto& pair : _radioButtons)
	{
		if (pair.second->GetValue())
		{
			return pair.first;
		}
	}

	return patch::CapType::Nocap; // invalid
}

std::string PatchCapDialog::getSelectedCapTypeString()
{
	switch (getSelectedCapType())
	{
	case patch::CapType::Bevel: return "bevel";
	case patch::CapType::EndCap: return "endcap";
	case patch::CapType::InvertedBevel: return "invertedbevel";
	case patch::CapType::InvertedEndCap: return "invertedendcap";
	case patch::CapType::Cylinder: return "cylinder";
	default: throw cmd::ExecutionFailure("Invalid cap type selected");
	};
}

void PatchCapDialog::Show(const cmd::ArgumentList & args)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("Cannot create caps, no patches selected."));
	}

	PatchCapDialog dialog;

	if (dialog.run() == IDialog::RESULT_OK)
	{
		GlobalCommandSystem().executeCommand("CapSelectedPatches", dialog.getSelectedCapTypeString());
	}
}

} // namespace ui
