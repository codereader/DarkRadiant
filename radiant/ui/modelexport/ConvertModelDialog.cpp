#include "ConvertModelDialog.h"

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
	const char* const WINDOW_TITLE = N_("Convert Model");

	const char* RKEY_MODEL_CONVERSION_CENTER_OBJECTS = "user/ui/convertModel/centerObjects";
	const char* RKEY_MODEL_CONVERSION_OUTPUT_PATH = "user/ui/convertModel/outputPath";
	const char* RKEY_MODEL_CONVERSION_OUTPUT_FORMAT = "user/ui/convertModel/outputFormat";
}

ConvertModelDialog::ConvertModelDialog(wxWindow* parent) :
	DialogBase(_(WINDOW_TITLE), parent)
{
	populateWindow();
}

void ConvertModelDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = loadNamedPanel(this, "ConvertModelDialogMainPanel");
	GetSizer()->Add(panel, 1, wxEXPAND);
	
	makeLabelBold(this, "InputPathLabel");
	makeLabelBold(this, "OutputPathLabel");
	makeLabelBold(this, "OptionsLabel");

	wxButton* exportButton = findNamedObject<wxButton>(this, "ConvertButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "CancelButton");

	exportButton->Bind(wxEVT_BUTTON, &ConvertModelDialog::onExport, this);
	cancelButton->Bind(wxEVT_BUTTON, &ConvertModelDialog::onCancel, this);

	wxChoice* formatChoice = findNamedObject<wxChoice>(this, "OutputFormatChoice");
	formatChoice->Clear();

	formatChoice->Bind(wxEVT_CHOICE, sigc::mem_fun(this, &ConvertModelDialog::onFormatSelection));

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

	std::string recentFormat = registry::getValue<std::string>(RKEY_MODEL_CONVERSION_OUTPUT_FORMAT);
	std::string recentPath = registry::getValue<std::string>(RKEY_MODEL_CONVERSION_OUTPUT_PATH);

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
    auto* existing = findNamedObject<wxWindow>(this, "OutputPathFilePicker");
	auto* pathEntry = new wxutil::PathEntry(existing->GetParent(), filetype::TYPE_MODEL_EXPORT, false, recentFormat);
    replaceControl(existing, pathEntry);

    pathEntry->setValue(recentPath);

	// We don't want the FileChooser to ask for permission overwriting an existing file,
	// we do this ourselves in this class when the user hits OK
	pathEntry->setAskForOverwrite(false);

	bool centerObjects = registry::getValue<bool>(RKEY_MODEL_CONVERSION_CENTER_OBJECTS);
	findNamedObject<wxCheckBox>(this, "CenterObjects")->SetValue(centerObjects);

	handleFormatSelectionChange();

	Layout();
	Fit();
	CenterOnScreen();
}

void ConvertModelDialog::onExport(wxCommandEvent& ev)
{
	bool centerObjects = findNamedObject<wxCheckBox>(this, "CenterObjects")->GetValue();
	std::string inputFilename = findNamedObject<wxutil::PathEntry>(this, "InputPathFilePicker")->getValue();
	std::string outputFilename = findNamedObject<wxutil::PathEntry>(this, "OutputPathFilePicker")->getValue();
	std::string outputFormat = wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "OutputFormatChoice"));

	if (outputFilename.empty())
	{
		wxutil::Messagebox::Show(_("Empty Filename"), _("No filename specified, cannot run exporter"), IDialog::MessageType::MESSAGE_ERROR);
		return;
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
		// ConvertModel <InputPath> <OutputPath> <ExportFormat> [<CenterObjects>]
		cmd::ArgumentList argList;

		argList.push_back(inputFilename);
		argList.push_back(outputFilename);
		argList.push_back(outputFormat);
		argList.push_back(centerObjects);

		GlobalCommandSystem().executeCommand("ConvertModel", argList);

		// Close the dialog
		EndModal(wxID_OK);
	}
	catch (std::runtime_error& ex)
	{
		wxutil::Messagebox::Show(_("Conversion failed"), ex.what(), IDialog::MessageType::MESSAGE_ERROR);
	}
}

void ConvertModelDialog::onCancel(wxCommandEvent& ev)
{
	// Remember stuff even when cancel is pressed
	saveOptionsToRegistry();

	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void ConvertModelDialog::handleFormatSelectionChange()
{
	std::string selectedFormat = wxutil::ChoiceHelper::GetSelectedStoredString(
		findNamedObject<wxChoice>(this, "OutputFormatChoice"));

	if (!selectedFormat.empty())
	{
		wxutil::PathEntry* pathEntry = findNamedObject<wxutil::PathEntry>(this, "OutputPathFilePicker");

		pathEntry->setDefaultExtension(selectedFormat);

		std::string extLower = string::to_lower_copy(selectedFormat);

		// Adjust the extension of the current file name
		if (!os::getExtension(pathEntry->getValue()).empty())
		{
			pathEntry->setValue(os::replaceExtension(pathEntry->getValue(), extLower));
		}
	}
}

void ConvertModelDialog::onFormatSelection(wxCommandEvent& ev)
{
	handleFormatSelectionChange();
}

bool ConvertModelDialog::_onDeleteEvent()
{
	// Remember stuff even when X is pressed
	saveOptionsToRegistry();

	return DialogBase::_onDeleteEvent();
}

void ConvertModelDialog::saveOptionsToRegistry()
{
	registry::setValue(RKEY_MODEL_CONVERSION_OUTPUT_FORMAT, 
		wxutil::ChoiceHelper::GetSelectedStoredString(findNamedObject<wxChoice>(this, "OutputFormatChoice")));

	registry::setValue(RKEY_MODEL_CONVERSION_OUTPUT_PATH,
		findNamedObject<wxutil::PathEntry>(this, "OutputPathFilePicker")->getValue());

	registry::setValue(RKEY_MODEL_CONVERSION_CENTER_OBJECTS, 
		findNamedObject<wxCheckBox>(this, "CenterObjects")->GetValue());
}

void ConvertModelDialog::ShowDialog(const cmd::ArgumentList& args)
{
	ConvertModelDialog* instance = new ConvertModelDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
