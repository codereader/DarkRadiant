#include "PatchCreateDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "selectionlib.h"
#include "string/convert.h"

#include <wx/choice.h>
#include <wx/checkbox.h>

namespace
{
	const char* WINDOW_TITLE = N_("Create Flat Patch Mesh");

	const bool DEFAULT_REMOVE_BRUSHES = false;

	const int MIN_PATCH_DIM = 3;
	const int MAX_PATCH_DIM = 15;
	const int INCR_PATCH_DIM = 2;
}

namespace ui
{

PatchCreateDialog::PatchCreateDialog() :
	Dialog(_(WINDOW_TITLE))
{
	_dialog->GetSizer()->Add(loadNamedPanel(_dialog, "PatchCreatePanel"), 1, wxEXPAND | wxALL, 12);

    // Create the title label (bold font)
	makeLabelBold(_dialog, "PatchCreateTopLabel");

	wxChoice* comboWidth = findNamedObject<wxChoice>(_dialog, "PatchCreateWidthChoice");
	wxChoice* comboHeight = findNamedObject<wxChoice>(_dialog, "PatchCreateHeightChoice");

	// Fill the values into the combo boxes
	for (int i = MIN_PATCH_DIM; i <= MAX_PATCH_DIM; i += INCR_PATCH_DIM)
	{
		comboWidth->Append(string::to_string(i));
		comboHeight->Append(string::to_string(i));
	}

	// Activate the first item in the combo boxes
	comboWidth->Select(0);
	comboHeight->Select(0);

	// Create the "remove brushes" label
	wxCheckBox* removeCheckBox = findNamedObject<wxCheckBox>(_dialog, "PatchCreateRemoveSelectedBrush");
	removeCheckBox->SetValue(DEFAULT_REMOVE_BRUSHES);
}

void PatchCreateDialog::construct()
{
	wxCheckBox* removeCheckBox = findNamedObject<wxCheckBox>(_dialog, "PatchCreateRemoveSelectedBrush");

	// Activate/Inactivate the check box depending on the selected brush count
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 1)
	{
		removeCheckBox->Enable(true);
		removeCheckBox->SetValue(true);
	}
	else
	{
		removeCheckBox->Enable(false);
		removeCheckBox->SetValue(false);
	}

	wxutil::Dialog::construct();
}

int PatchCreateDialog::getSelectedWidth()
{
	wxChoice* comboWidth = findNamedObject<wxChoice>(_dialog, "PatchCreateWidthChoice");
	return string::convert<int>(comboWidth->GetStringSelection().ToStdString());
}

int PatchCreateDialog::getSelectedHeight()
{
	wxChoice* comboHeight = findNamedObject<wxChoice>(_dialog, "PatchCreateHeightChoice");
	return string::convert<int>(comboHeight->GetStringSelection().ToStdString());
}

bool PatchCreateDialog::getRemoveSelectedBrush()
{
	return findNamedObject<wxCheckBox>(_dialog, "PatchCreateRemoveSelectedBrush")->GetValue();
}

} // namespace ui
