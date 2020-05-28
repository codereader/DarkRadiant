#include "PatchThickenDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "string/convert.h"
#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"

#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>

namespace
{
	const char* const WINDOW_TITLE = N_("Patch Thicken");
}

namespace ui
{

PatchThickenDialog::PatchThickenDialog() :
	Dialog(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow())
{
	_dialog->GetSizer()->Add(loadNamedPanel(_dialog, "ThickenDialogMainPanel"), 1, wxEXPAND | wxALL, 12);

	wxStaticText* topLabel = findNamedObject<wxStaticText>(_dialog, "ThickenDialogTopLabel");
	topLabel->SetFont(topLabel->GetFont().Bold());
}

float PatchThickenDialog::getThickness()
{
	wxTextCtrl* entry = findNamedObject<wxTextCtrl>(_dialog, "ThickenDialogThickness");
	return string::convert<float>(entry->GetValue().ToStdString(), 0.0f);
}

bool PatchThickenDialog::getCeateSeams()
{
	return findNamedObject<wxCheckBox>(_dialog, "ThickenDialogCreateSeams")->GetValue();
}

int PatchThickenDialog::getAxis()
{
	if (findNamedObject<wxRadioButton>(_dialog, "ThickenDialogExtrudeAlongX")->GetValue())
	{
		return 0;
	}
	else if (findNamedObject<wxRadioButton>(_dialog, "ThickenDialogExtrudeAlongY")->GetValue())
	{
		return 1;
	}
	else if (findNamedObject<wxRadioButton>(_dialog, "ThickenDialogExtrudeAlongZ")->GetValue())
	{
		return 2;
	}
	else
	{
		// Extrude along normals
		return 3;
	}
}

void PatchThickenDialog::Show(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		throw cmd::ExecutionNotPossible(_("Cannot thicken patch. Nothing selected."));
	}

	PatchThickenDialog dialog;

	if (dialog.run() == IDialog::RESULT_OK)
	{
		GlobalCommandSystem().executeCommand("ThickenSelectedPatches",
			{ cmd::Argument(dialog.getThickness()), 
			  cmd::Argument(dialog.getCeateSeams() ? 1 : 0),
			  cmd::Argument(dialog.getAxis()) }
		);
	}
}

} // namespace ui
