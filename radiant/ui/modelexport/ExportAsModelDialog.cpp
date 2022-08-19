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
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
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
	constexpr const char* const WINDOW_TITLE = N_("Model Export");

	constexpr const char* const RKEY_MODEL_EXPORT_SKIP_CAULK = "user/ui/exportAsModel/skipCaulk";
	constexpr const char* const RKEY_MODEL_EXPORT_EXPORT_ORIGIN = "user/ui/exportAsModel/exportOrigin";
	constexpr const char* const RKEY_MODEL_EXPORT_CUSTOM_ORIGIN = "user/ui/exportAsModel/customOrigin";
	constexpr const char* const RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL = "user/ui/exportAsModel/replaceSelectionWithModel";
	constexpr const char* const RKEY_MODEL_EXPORT_OUTPUT_PATH = "user/ui/exportAsModel/outputPath";
	constexpr const char* const RKEY_MODEL_EXPORT_OUTPUT_FORMAT = "user/ui/exportAsModel/outputFormat";
	constexpr const char* const RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS = "user/ui/exportAsModel/exportLightsAsObjects";
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
	makeLabelBold(this, "ExportDialogOriginLabel");
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

    auto customOrigin = registry::getValue<std::string>(RKEY_MODEL_EXPORT_CUSTOM_ORIGIN);
    findNamedObject<wxTextCtrl>(this, "ExportDialogCustomOrigin")->SetValue(customOrigin);

    // Populate the available entitys
    auto entitySelector = findNamedObject<wxChoice>(this, "ExportDialogEntitySelector");
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        if (!Node_isEntity(node)) return;

        auto caption = fmt::format("{0} at ({1})",
            Node_getEntity(node)->getKeyValue("name"),
            Node_getEntity(node)->getKeyValue("origin"));

        // Store the entity name as client data
        entitySelector->Append(caption, new wxStringClientData(Node_getEntity(node)->getKeyValue("name")));
    });

    if (entitySelector->GetCount() > 0)
    {
        entitySelector->Select(0);
    }

    auto exportOrigin = static_cast<model::ModelExportOrigin>(registry::getValue<int>(RKEY_MODEL_EXPORT_EXPORT_ORIGIN));
    setSelectedExportOrigin(exportOrigin);

	if (entitySelector->GetCount() > 0)
	{
        findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->Enable(true);
        findNamedObject<wxChoice>(this, "ExportDialogEntitySelector")->Enable(true);
	}
	else
	{
        findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->Enable(false);
        findNamedObject<wxChoice>(this, "ExportDialogEntitySelector")->Enable(false);

        // Switch to another option
        if (getSelectedExportOrigin() == model::ModelExportOrigin::EntityOrigin)
        {
            setSelectedExportOrigin(model::ModelExportOrigin::SelectionCenter);
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
	auto customOrigin = findNamedObject<wxTextCtrl>(this, "ExportDialogCustomOrigin")->GetValue().ToStdString();
    auto entityName = std::string();

    // Validate the user-entered origin
    if (exportOrigin == model::ModelExportOrigin::CustomOrigin)
    {
        auto invalidOrigin = Vector3(65536, 65536, 65536);
        if (string::convert<Vector3>(customOrigin, invalidOrigin) == invalidOrigin)
        {
            wxutil::Messagebox::Show(_("Invalid Origin"), 
                _("The origin you entered could not be parsed.\nUse the format \"x y z\" (without quotes, separated with spaces)"), 
                IDialog::MessageType::MESSAGE_ERROR);
            return;
        }
    }
    else if (exportOrigin == model::ModelExportOrigin::EntityOrigin)
    {
        entityName = wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "ExportDialogEntitySelector"));
    }

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
		// ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>] [<SkipCaulk>] [<ReplaceSelectionWithModel>] [<ExportLightsAsObjects>]
		cmd::ArgumentList argList;

		argList.push_back(outputFilename);
		argList.push_back(outputFormat);
		argList.push_back(model::getExportOriginString(exportOrigin));
		argList.push_back(entityName);
		argList.push_back(string::convert<Vector3>(customOrigin));
		argList.push_back(skipCaulk);
		argList.push_back(replaceSelectionWithModel);
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

	registry::setValue(RKEY_MODEL_EXPORT_CUSTOM_ORIGIN,
        findNamedObject<wxTextCtrl>(this, "ExportDialogCustomOrigin")->GetValue().ToStdString());

	registry::setValue(RKEY_MODEL_EXPORT_REPLACE_WITH_MODEL,
		findNamedObject<wxCheckBox>(this, "ExportDialogReplaceWithModel")->GetValue());

	registry::setValue(RKEY_MODEL_EXPORT_EXPORT_LIGHTS_AS_OBJECTS,
		findNamedObject<wxCheckBox>(this, "ExportDialogExportLightsAsObjects")->GetValue());
}

model::ModelExportOrigin ExportAsModelDialog::getSelectedExportOrigin()
{
    if (findNamedObject<wxRadioButton>(this, "ExportOriginUseMapOrigin")->GetValue())
    {
        return model::ModelExportOrigin::MapOrigin;
    }
    else if (findNamedObject<wxRadioButton>(this, "ExportOriginUseSelectionCenter")->GetValue())
    {
        return model::ModelExportOrigin::SelectionCenter;
    }
    else if (findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->GetValue())
    {
        return model::ModelExportOrigin::EntityOrigin;
    }
    else if (findNamedObject<wxRadioButton>(this, "ExportOriginUseCustomOrigin")->GetValue())
    {
        return model::ModelExportOrigin::CustomOrigin;
    }

    return model::ModelExportOrigin::MapOrigin;
}

void ExportAsModelDialog::setSelectedExportOrigin(model::ModelExportOrigin exportOrigin)
{
    findNamedObject<wxRadioButton>(this, "ExportOriginUseMapOrigin")->SetValue(exportOrigin == model::ModelExportOrigin::MapOrigin);
    findNamedObject<wxRadioButton>(this, "ExportOriginUseSelectionCenter")->SetValue(exportOrigin == model::ModelExportOrigin::SelectionCenter);
    findNamedObject<wxRadioButton>(this, "ExportOriginUseEntityOrigin")->SetValue(exportOrigin == model::ModelExportOrigin::EntityOrigin);
    findNamedObject<wxRadioButton>(this, "ExportOriginUseCustomOrigin")->SetValue(exportOrigin == model::ModelExportOrigin::CustomOrigin);
}

void ExportAsModelDialog::ShowDialog(const cmd::ArgumentList&)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		wxutil::Messagebox::Show(_("Empty Selection"), _("Nothing selected, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
	}

	auto instance = new ExportAsModelDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
