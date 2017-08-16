#include "ExportAsModelDialog.h"

#include "i18n.h"
#include "imodel.h"
#include "iselection.h"
#include "ifiletypes.h"

#include <stdexcept>
#include <sigc++/functors/mem_fun.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <boost/algorithm/string/case_conv.hpp>

#include "registry/registry.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/ChoiceHelper.h"
#include "wxutil/PathEntry.h"
#include "map/algorithm/Export.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Model Export");

	const char* RKEY_MODEL_EXPORT_SKIP_CAULK = "user/ui/exportAsModel/skipCaulk";
	const char* RKEY_MODEL_EXPORT_CENTER_OBJECTS = "user/ui/exportAsModel/centerObjects";
	const char* RKEY_MODEL_EXPORT_OUTPUT_PATH = "user/ui/exportAsModel/outputPath";
	const char* RKEY_MODEL_EXPORT_OUTPUT_FORMAT = "user/ui/exportAsModel/outputFormat";
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
	formatChoice->Clear();

	formatChoice->Bind(wxEVT_CHOICE, sigc::mem_fun(this, &ExportAsModelDialog::onFormatSelection));

	// Push the available formats to the wxChoice control
	GlobalModelFormatManager().foreachExporter([&](const model::IModelExporterPtr& exporter)
	{
		// Generate the display name "<exporterName> (.<ext>)"
		std::string displayName = exporter->getDisplayName();
		displayName += " (." + boost::algorithm::to_lower_copy(exporter->getExtension()) + ")";

		// Store the exporter extension as client data
		formatChoice->Append(displayName, new wxStringClientData(exporter->getExtension()));
	});

	// Select the first format for starters
	formatChoice->Select(0);

	std::string recentFormat = registry::getValue<std::string>(RKEY_MODEL_EXPORT_OUTPUT_FORMAT);
	std::string recentPath = registry::getValue<std::string>(RKEY_MODEL_EXPORT_OUTPUT_PATH);

	if (!recentFormat.empty())
	{
		wxutil::ChoiceHelper::SelectItemByStoredString(formatChoice, recentFormat);
	}

	// Replace the filepicker control with our own PathEntry
	wxWindow* existing = findNamedObject<wxWindow>(this, "ExportDialogFilePicker");

	wxutil::PathEntry* pathEntry = new wxutil::PathEntry(existing->GetParent(), filetype::TYPE_MODEL_EXPORT);
	pathEntry->setValue(recentPath);
	pathEntry->SetName("ExportDialogFilePicker");
	pathEntry->setDefaultExtension(recentFormat);

	existing->GetContainingSizer()->Replace(existing, pathEntry);
	existing->Destroy();

	bool skipCaulk = registry::getValue<bool>(RKEY_MODEL_EXPORT_SKIP_CAULK);
	findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->SetValue(skipCaulk);

	bool centerObjects = registry::getValue<bool>(RKEY_MODEL_EXPORT_CENTER_OBJECTS);
	findNamedObject<wxCheckBox>(this, "ExportDialogCenterObjects")->SetValue(centerObjects);

	Layout();
	Fit();
	CenterOnScreen();
}

void ExportAsModelDialog::onExport(wxCommandEvent& ev)
{
	map::algorithm::ModelExportOptions options;

	options.centerObjects = findNamedObject<wxCheckBox>(this, "ExportDialogCenterObjects")->GetValue();
	options.skipCaulk = findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->GetValue();
	options.outputFilename = findNamedObject<wxutil::PathEntry>(this, "ExportDialogFilePicker")->getValue();
	options.outputFormat = wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "ExportDialogFormatChoice"));

	if (options.outputFilename.empty())
	{
		wxutil::Messagebox::Show(_("Empty Filename"), _("No filename specified, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

	saveOptionsToRegistry();

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
	// Remember stuff even when cancel is pressed
	saveOptionsToRegistry();

	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void ExportAsModelDialog::onFormatSelection(wxCommandEvent& ev)
{
	std::string selectedFormat = wxutil::ChoiceHelper::GetSelectedStoredString(
		findNamedObject<wxChoice>(this, "ExportDialogFormatChoice"));

	if (!selectedFormat.empty())
	{
		findNamedObject<wxutil::PathEntry>(this, "ExportDialogFilePicker")->setDefaultExtension(selectedFormat);
	}
}

bool ExportAsModelDialog::_onDeleteEvent()
{
	// Remember stuff even when X is pressed
	saveOptionsToRegistry();

	return DialogBase::_onDeleteEvent();
}

void ExportAsModelDialog::saveOptionsToRegistry()
{
	registry::setValue(RKEY_MODEL_EXPORT_OUTPUT_FORMAT, 
		wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "ExportDialogFormatChoice")));

	registry::setValue(RKEY_MODEL_EXPORT_OUTPUT_PATH, 
		findNamedObject<wxutil::PathEntry>(this, "ExportDialogFilePicker")->getValue());

	registry::setValue(RKEY_MODEL_EXPORT_SKIP_CAULK, 
		findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->GetValue());

	registry::setValue(RKEY_MODEL_EXPORT_CENTER_OBJECTS, 
		findNamedObject<wxCheckBox>(this, "ExportDialogCenterObjects")->GetValue());
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
