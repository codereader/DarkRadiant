#include "PatchThickenDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "string/convert.h"

#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>

namespace
{
	const char* const WINDOW_TITLE = N_("Patch Thicken");

	const float DEFAULT_THICKNESS = 16.0f;
	const bool DEFAULT_CREATE_SEAMS = TRUE;
}

namespace ui
{

PatchThickenDialog::PatchThickenDialog() :
	Dialog(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow())
{
	GetSizer()->Add(loadNamedPanel(this, "ThickenDialogMainPanel"), 1, wxEXPAND | wxALL, 12);

	wxStaticText* topLabel = findNamedObject<wxStaticText>(this, "ThickenDialogTopLabel");
	topLabel->SetFont(topLabel->GetFont().Bold());
}

float PatchThickenDialog::getThickness()
{
	wxTextCtrl* entry = findNamedObject<wxTextCtrl>(this, "ThickenDialogThickness");
	return string::convert<float>(entry->GetValue(), 0.0f);
}

bool PatchThickenDialog::getCeateSeams()
{
	return findNamedObject<wxCheckBox>(this, "ThickenDialogCreateSeams")->GetValue();
}

int PatchThickenDialog::getAxis()
{
	if (findNamedObject<wxRadioButton>(this, "ThickenDialogExtrudeAlongX")->GetValue())
	{
		return 0;
	}
	else if (findNamedObject<wxRadioButton>(this, "ThickenDialogExtrudeAlongY")->GetValue())
	{
		return 1;
	}
	else if (findNamedObject<wxRadioButton>(this, "ThickenDialogExtrudeAlongZ")->GetValue())
	{
		return 2;
	}
	else
	{
		// Extrude along normals
		return 3;
	}
}

} // namespace ui
