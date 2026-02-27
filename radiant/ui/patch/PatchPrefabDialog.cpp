#include "PatchPrefabDialog.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "selectionlib.h"

#include "wxutil/dialog/DialogBase.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Create Patch Prefab");
}

void PatchPrefabDialog::Show(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: CreatePrefabPatchDialog <type>" << std::endl;

		return;
	}

	std::string prefabType = args[0].getString();
	int brushCount = GlobalSelectionSystem().getSelectionInfo().brushCount;
	if (brushCount == 0)
	{
		GlobalCommandSystem().executeCommand("CreatePatchPrefab", { cmd::Argument(prefabType) });

		return;
	}

	auto* dialog = new wxutil::DialogBase(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow());
	dialog->SetSizer(new wxBoxSizer(wxVERTICAL));

	auto* checkbox = new wxCheckBox(dialog, wxID_ANY, _("Remove selected Brush"));
	dialog->GetSizer()->Add(checkbox, 0, wxALL, 12);
	dialog->GetSizer()->Add(dialog->CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

	if (brushCount == 1)
	{
		checkbox->Enable(true);
		checkbox->SetValue(true);
	}
	else
	{
		checkbox->Enable(false);
		checkbox->SetValue(false);
	}

	dialog->Fit();
	dialog->CentreOnScreen();

	bool removeSelectedBrush = false;

	if (dialog->ShowModal() == wxID_OK)
	{
		removeSelectedBrush = checkbox->GetValue();
	}
	else
	{
		dialog->Destroy();
		return;
	}

	dialog->Destroy();

	GlobalCommandSystem().executeCommand("CreatePatchPrefab",
		{ cmd::Argument(prefabType), cmd::Argument(removeSelectedBrush ? 1 : 0) });
}

} // namespace ui
