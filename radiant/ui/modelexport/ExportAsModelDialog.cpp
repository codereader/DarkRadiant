#include "ExportAsModelDialog.h"

#include "i18n.h"
#include "imodel.h"
#include "iselection.h"
#include "ifiletypes.h"
#include "icommandsystem.h"
#include "igame.h"

#include <stdexcept>
#include <sigc++/functors/mem_fun.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
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

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Model Export");

	const char* RKEY_MODEL_EXPORT_SKIP_CAULK = "user/ui/exportAsModel/skipCaulk";
	const char* RKEY_MODEL_EXPORT_EXPORT_ORIGIN = "user/ui/exportAsModel/exportOrigin";
	const char* RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL = "user/ui/exportAsModel/replaceSelectionWithModel";
	const char* RKEY_MODEL_EXPORT_OUTPUT_PATH = "user/ui/exportAsModel/outputPath";
	const char* RKEY_MODEL_EXPORT_OUTPUT_FORMAT = "user/ui/exportAsModel/outputFormat";
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

	auto exportButton = findNamedObject<wxButton>(this, "ExportDialogExportButton");
	auto cancelButton = findNamedObject<wxButton>(this, "ExportDialogCancelButton");

	exportButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &ExportAsModelDialog::onExport));
	cancelButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &ExportAsModelDialog::onCancel));

	auto formatChoice = findNamedObject<wxChoice>(this, "ExportDialogFormatChoice");
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

	bool replaceSelectionWithModel = registry::getValue<bool>(RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL);
	findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->SetValue(replaceSelectionWithModel);

	bool exportLightsAsObjects = registry::getValue<bool>(RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS);
	findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->SetValue(exportLightsAsObjects);

    auto exportOrigin = static_cast<ExportOrigin>(registry::getValue<int>(RKEY_MODEL_EXPORT_EXPORT_ORIGIN));
    setSelectedExportOrigin(exportOrigin);

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 1 && info.entityCount == 1)
	{
        findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->Enable(true);
	}
	else
	{
        findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->Enable(false);

        // Switch to another option
        if (getSelectedExportOrigin() == ExportOrigin::EntityOrigin)
        {
            setSelectedExportOrigin(ExportOrigin::SelectionCenter);
        }
	}

	// Check if options are available for the current format
	handleFormatSelectionChange();

	Layout();
	Fit();
	CenterOnScreen();
}

void ExportAsModelDialog::onExport(wxCommandEvent& ev)
{
    auto exportOrigin = getSelectedExportOrigin();
	bool skipCaulk = findNamedObject<wxCheckBox>(this, "ExportDialogSkipCaulk")->GetValue();
	std::string outputFilename = findNamedObject<wxutil::PathEntry>(this, "ExportDialogFilePicker")->getValue();
	std::string outputFormat = wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "ExportDialogFormatChoice"));
	bool replaceSelectionWithModel = findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->GetValue();
	bool exportLightsAsObjects = findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->GetValue();

	if (outputFilename.empty())
	{
		wxutil::Messagebox::Show(_("Empty Filename"), _("No filename specified, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

    // Warn the user if the output file extension doesn't match what the format says (#5741)
    if (string::to_lower_copy(os::getExtension(outputFilename)) != string::to_lower_copy(outputFormat) &&
        wxutil::Messagebox::Show(_("Format/Extension Mismatch"), _("The file extension doesn't match the selected format - continue?"), 
            IDialog::MessageType::MESSAGE_ASK) == IDialog::RESULT_NO)
    {
        return; // abort
    }

	// Check if the target file already exists
	if (os::fileOrDirExists(outputFilename) &&
		wxutil::Messagebox::Show(_("Confirm Replacement"), 
			fmt::format(_("The file {0} already exists.\nReplace this file?"), outputFilename),
			IDialog::MessageType::MESSAGE_ASK) != IDialog::RESULT_YES)
	{
		return; // abort
	}

	saveOptionsToRegistry();

	try
	{
		// ExportSelectedAsModel <Path> <ExportFormat> [<CenterObjects>] [<SkipCaulk>] [<ReplaceSelectionWithModel>] [<UseEntityOrigin>] [<ExportLightsAsObjects>]
		cmd::ArgumentList argList;

		argList.push_back(outputFilename);
		argList.push_back(outputFormat);
		argList.push_back(exportOrigin == ExportOrigin::SelectionCenter);
		argList.push_back(skipCaulk);
		argList.push_back(replaceSelectionWithModel);
		argList.push_back(exportOrigin == ExportOrigin::EntityOrigin);
		argList.push_back(exportLightsAsObjects);

		GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

		// Close the dialog
		EndModal(wxID_OK);
	}
	catch (const std::runtime_error& ex)
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
		pathEntry->setValue(os::replaceExtension(pathEntry->getValue(), extLower));

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

	registry::setValue(RKEY_MODEL_EXPORT_EXPORT_ORIGIN, static_cast<int>(getSelectedExportOrigin()));

	registry::setValue(RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL,
		findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->GetValue());

	registry::setValue(RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS,
		findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->GetValue());
}

ExportAsModelDialog::ExportOrigin ExportAsModelDialog::getSelectedExportOrigin()
{
    if (findNamedObject<wxRadioButton>(this, "ExportOriginUseMapOrigin")->GetValue())
    {
        return ExportOrigin::MapOrigin;
    }
    else if (findNamedObject<wxRadioButton>(this, "ExportOriginUseSelectionCenter")->GetValue())
    {
        return ExportOrigin::SelectionCenter;
    }
    else if (findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->GetValue())
    {
        return ExportOrigin::EntityOrigin;
    }

    return ExportOrigin::MapOrigin;
}

void ExportAsModelDialog::setSelectedExportOrigin(ExportOrigin exportOrigin)
{
    findNamedObject<wxRadioButton>(this, "ExportOriginUseMapOrigin")->SetValue(exportOrigin == ExportOrigin::MapOrigin);
    findNamedObject<wxRadioButton>(this, "ExportOriginUseSelectionCenter")->SetValue(exportOrigin == ExportOrigin::SelectionCenter);
    findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->SetValue(exportOrigin == ExportOrigin::EntityOrigin);
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
