#include "FindShader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"

#include "ui/common/ShaderChooser.h"
#include "selection/algorithm/Shader.h"

#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>

namespace ui
{

namespace
{
	const char* const FINDDLG_WINDOW_TITLE = N_("Find & Replace Shader");
	const char* const COUNT_TEXT = N_("%d shader(s) replaced.");
}

FindAndReplaceShader::FindAndReplaceShader() :
	wxutil::DialogBase(_(FINDDLG_WINDOW_TITLE))
{
	// Create all the widgets
	populateWindow();

	Fit();
	CenterOnParent();
}

FindAndReplaceShader::~FindAndReplaceShader()
{
}

void FindAndReplaceShader::populateWindow()
{
	wxPanel* mainPanel = loadNamedPanel(this, "FindReplaceDialogMainPanel");

	findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(FindAndReplaceShader::onEntryChanged), NULL, this);
	findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(FindAndReplaceShader::onEntryChanged), NULL, this);

	findNamedObject<wxButton>(this, "FindReplaceDialogFindSelectButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChooseFind), NULL, this);
	findNamedObject<wxButton>(this, "FindReplaceDialogReplaceSelectButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChooseReplace), NULL, this);

	findNamedObject<wxButton>(this, "FindReplaceDialogFindButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onReplace), NULL, this);
	findNamedObject<wxButton>(this, "FindReplaceDialogCloseButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onClose), NULL, this);

	findNamedObject<wxStaticText>(this, "FindReplaceDialogStatusLabel")->SetLabel("");

	SetSize(mainPanel->GetMinSize());
}

void FindAndReplaceShader::performReplace()
{
	const std::string find = findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry")->GetValue().ToStdString();
	const std::string replace = findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry")->GetValue().ToStdString();

	bool selectedOnly = findNamedObject<wxCheckBox>(this, "FindReplaceDialogSearchCurSelection")->GetValue();

	int replaced = selection::algorithm::findAndReplaceShader(find, replace, selectedOnly);

	wxStaticText* status = findNamedObject<wxStaticText>(this, "FindReplaceDialogStatusLabel");
	status->SetLabel((boost::format(_(COUNT_TEXT)) % replaced).str());
}

void FindAndReplaceShader::onChooseFind(wxCommandEvent& ev)
{
	// Construct the modal dialog
	ShaderChooser* chooser = new ShaderChooser(this, 
		findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry"));

	chooser->ShowModal();
	chooser->Destroy();
}

void FindAndReplaceShader::onChooseReplace(wxCommandEvent& ev)
{
	// Construct the modal dialog
	ShaderChooser* chooser = new ShaderChooser(this, 
		findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry"));

	chooser->ShowModal();
	chooser->Destroy();
}

void FindAndReplaceShader::onReplace(wxCommandEvent& ev)
{
	performReplace();
}

void FindAndReplaceShader::onClose(wxCommandEvent& ev)
{
	EndModal(wxID_CLOSE);
}

void FindAndReplaceShader::onEntryChanged(wxCommandEvent& ev)
{
	findNamedObject<wxStaticText>(this, "FindReplaceDialogStatusLabel")->SetLabel("");
}

void FindAndReplaceShader::ShowDialog(const cmd::ArgumentList& args)
{
	// Just instantiate a new dialog, this enters a main loop
	FindAndReplaceShader* dialog = new FindAndReplaceShader;

	dialog->ShowModal();
	dialog->Destroy();
}

} // namespace ui
