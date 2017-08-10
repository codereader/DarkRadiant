#include "ExportAsModelDialog.h"

#include "i18n.h"
#include "imodel.h"
#include "iselection.h"

#include <stdexcept>
#include <sigc++/functors/mem_fun.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/sizer.h>

#include "wxutil/dialog/MessageBox.h"
#include "wxutil/ChoiceHelper.h"
#include "map/algorithm/Export.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Model Export");
}

ExportAsModelDialog::ExportAsModelDialog(wxWindow* parent) :
	DialogBase(_(WINDOW_TITLE), parent)
{
	populateWindow();
}

void ExportAsModelDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = loadNamedPanel(this, "ExportDialogMainPanel");
	GetSizer()->Add(panel, 1, wxEXPAND);
	
	makeLabelBold(this, "ExportDialogOutputLabel");
	makeLabelBold(this, "ExportDialogOptionLabel");

	wxButton* exportButton = findNamedObject<wxButton>(this, "ExportDialogExportButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "ExportDialogCancelButton");

	exportButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &ExportAsModelDialog::onExport));
	cancelButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &ExportAsModelDialog::onCancel));

	wxChoice* formatChoice = findNamedObject<wxChoice>(this, "ExportDialogFormatChoice");

	// Push the available formats to the wxChoice control
	GlobalModelFormatManager().foreachExporter([&](const model::IModelExporterPtr& exporter)
	{
		// Store the exporter extension as client data
		formatChoice->Append(exporter->getDisplayName(), new wxStringClientData(exporter->getExtension()));
	});

	Layout();
	Fit();
	CenterOnScreen();
}

void ExportAsModelDialog::onExport(wxCommandEvent& ev)
{
	map::algorithm::ModelExportOptions options;

	options.centerObjects = findNamedObject<wxCheckBox>(this, "ExportDialogCenterObjects")->GetValue();
	options.skipCaulk = findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->GetValue();
	options.outputFilename = findNamedObject<wxFilePickerCtrl>(this, "ExportDialogFilePicker")->GetFileName().GetFullPath();
	options.outputFormat = wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "ExportDialogFormatChoice"));

	if (options.outputFilename.empty())
	{
		wxutil::Messagebox::Show(_("Empty Filename"), _("No filename specified, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

	try
	{
		map::algorithm::exportSelectedAsModel(options);

		// Close the dialog
		EndModal(wxID_OK);
	}
	catch (std::runtime_error& ex)
	{
		wxutil::Messagebox::Show(_("Export failed"), ex.what(), IDialog::MessageType::MESSAGE_ERROR);
	}
}

void ExportAsModelDialog::onCancel(wxCommandEvent& ev)
{
	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void ExportAsModelDialog::ShowDialog(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		wxutil::Messagebox::Show(_("Empty Selection"), _("Nothing selected, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

	ExportAsModelDialog* instance = new ExportAsModelDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
