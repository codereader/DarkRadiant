#include "ExportAsModelDialog.h"

#include "i18n.h"
#include "imodel.h"
#include "iselection.h"
#include "ifiletypes.h"
#include "igame.h"

#include <stdexcept>
#include <sigc++/functors/mem_fun.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include "string/case_conv.h"
#include "string/split.h"

#include "selectionlib.h"
#include "os/path.h"
#include "os/file.h"
#include "os/dir.h"
#include "os/fs.h"
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
	const char* RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL = "user/ui/exportAsModel/replaceSelectionWithModel";
	const char* RKEY_MODEL_EXPORT_OUTPUT_PATH = "user/ui/exportAsModel/outputPath";
	const char* RKEY_MODEL_EXPORT_OUTPUT_FORMAT = "user/ui/exportAsModel/outputFormat";
	const char* RKEY_MODEL_EXPORT_USE_ENTITY_ORIGIN = "user/ui/exportAsModel/keepEntityOrigin";
	const char* RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS = "user/ui/exportAsModel/exportLightsAsObjects";
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
		displayName += " (." + string::to_lower_copy(exporter->getExtension()) + ")";

		// Store the exporter extension as client data
		formatChoice->Append(displayName, new wxStringClientData(exporter->getExtension()));
	});

	// Select the first format for starters
	formatChoice->Select(0);

	std::string recentFormat = registry::getValue<std::string>(RKEY_MODEL_EXPORT_OUTPUT_FORMAT);
	std::string recentPath = registry::getValue<std::string>(RKEY_MODEL_EXPORT_OUTPUT_PATH);

	// Default to the models path of the current mod or game
	if (recentPath.empty())
	{
		recentPath = GlobalGameManager().getModPath();

		if (recentPath.empty())
		{
			recentPath = GlobalGameManager().getUserEnginePath();
		}

		recentPath = os::standardPathWithSlash(recentPath) + "models/";

		if (!os::fileOrDirExists(recentPath))
		{
			rMessage() << "Creating default model output folder: " << recentPath << std::endl;

			os::makeDirectory(recentPath);
		}
	}

	if (!recentFormat.empty())
	{
		wxutil::ChoiceHelper::SelectItemByStoredString(formatChoice, recentFormat);
	}

	// Replace the filepicker control with our own PathEntry
	wxWindow* existing = findNamedObject<wxWindow>(this, "ExportDialogFilePicker");

	wxutil::PathEntry* pathEntry = new wxutil::PathEntry(existing->GetParent(), 
		filetype::TYPE_MODEL_EXPORT, false, recentFormat);

	pathEntry->setValue(recentPath);
	pathEntry->SetName("ExportDialogFilePicker");

	// We don't want the FileChooser to ask for permission overwriting an existing file,
	// we do this ourselves in this class when the user hits OK
	pathEntry->setAskForOverwrite(false);

	existing->GetContainingSizer()->Replace(existing, pathEntry);
	existing->Destroy();

	bool skipCaulk = registry::getValue<bool>(RKEY_MODEL_EXPORT_SKIP_CAULK);
	findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->SetValue(skipCaulk);

	bool centerObjects = registry::getValue<bool>(RKEY_MODEL_EXPORT_CENTER_OBJECTS);
	findNamedObject<wxCheckBox>(this, "ExportDialogCenterObjects")->SetValue(centerObjects);

	bool replaceSelectionWithModel = registry::getValue<bool>(RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL);
	findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->SetValue(replaceSelectionWithModel);

	bool keepEntityOrigin = registry::getValue<bool>(RKEY_MODEL_EXPORT_USE_ENTITY_ORIGIN);
	wxCheckBox* keepOriginBox = findNamedObject<wxCheckBox>(this, "ExportDialogUseEntityOrigin");

	bool exportLightsAsObjects = registry::getValue<bool>(RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS);
	findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->SetValue(exportLightsAsObjects);

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 1 && info.entityCount == 1)
	{
		keepOriginBox->SetValue(keepEntityOrigin);
		keepOriginBox->Enable(true);
	}
	else
	{
		keepOriginBox->SetValue(false);
		keepOriginBox->Enable(false);
	}

	// Check if options are available for the current format
	handleFormatSelectionChange();

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
	options.replaceSelectionWithModel = findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->GetValue();
	options.useEntityOrigin = findNamedObject<wxCheckBox>(this, "ExportDialogUseEntityOrigin")->GetValue();
	options.exportLightsAsObjects = findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->GetValue();

	if (options.outputFilename.empty())
	{
		wxutil::Messagebox::Show(_("Empty Filename"), _("No filename specified, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

	// Check if the target file already exists
	if (os::fileOrDirExists(options.outputFilename) && 
		wxutil::Messagebox::Show(_("Confirm Replacement"), 
			fmt::format(_("The file {0} already exists.\nReplace this file?"), options.outputFilename),
			IDialog::MessageType::MESSAGE_ASK) != IDialog::RESULT_YES)
	{
		return; // abort
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

void ExportAsModelDialog::handleFormatSelectionChange()
{
	std::string selectedFormat = wxutil::ChoiceHelper::GetSelectedStoredString(
		findNamedObject<wxChoice>(this, "ExportDialogFormatChoice"));

	if (!selectedFormat.empty())
	{
		wxutil::PathEntry* pathEntry = findNamedObject<wxutil::PathEntry>(this, "ExportDialogFilePicker");

		pathEntry->setDefaultExtension(selectedFormat);

		std::string extLower = string::to_lower_copy(selectedFormat);

		// Adjust the extension of the current file name
		if (!os::getExtension(pathEntry->getValue()).empty())
		{
			pathEntry->setValue(os::replaceExtension(pathEntry->getValue(), extLower));
		}

		// Check if the replace current selection option is available
		std::string extensions = GlobalGameManager().currentGame()->getKeyValue("modeltypes");
		std::set<std::string> supportedExtensions;
		string::split(supportedExtensions, extensions, " ");

		wxCheckBox* replaceSelectionBox = findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel");

		// If the current game supports the format, make the option available
		bool formatSupportedByCurrentGame = supportedExtensions.find(extLower) != supportedExtensions.end();

		replaceSelectionBox->Enable(formatSupportedByCurrentGame);

		if (!formatSupportedByCurrentGame)
		{
			replaceSelectionBox->SetValue(false);
		}
	}
}

void ExportAsModelDialog::onFormatSelection(wxCommandEvent& ev)
{
	handleFormatSelectionChange();
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

	registry::setValue(RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL,
		findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->GetValue());

	registry::setValue(RKEY_MODEL_EXPORT_USE_ENTITY_ORIGIN,
		findNamedObject<wxCheckBox>(this, "ExportDialogUseEntityOrigin")->GetValue());

	registry::setValue(RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS,
		findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->GetValue());
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
