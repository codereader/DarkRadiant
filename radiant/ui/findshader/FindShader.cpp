#include "FindShader.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "imousetoolmanager.h"

#include "ui/common/ShaderChooser.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/algorithm/Shader.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"
#include "camera/tools/ShaderClipboardTools.h"
#include "registry/registry.h"

#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <fmt/format.h>

namespace ui
{

namespace
{
	const char* const FINDDLG_WINDOW_TITLE = N_("Find & Replace Shader");
	const char* const COUNT_TEXT = N_("{0:d} shader(s) replaced.");

    const std::string RKEY_ROOT = "user/ui/textures/findShaderDialog/";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
    const std::string RKEY_PICK_HINT_SHOWN = RKEY_ROOT + "pickHintShown";

    const char* const PICK_TEXTURE_HINT = N_("When picking texture names, click the pick button and use {0}\n"
                                             "in the camera view to pick a material name. The picked texture will be\n"
                                             "filled in the entry box next to the activated button.");
}

FindAndReplaceShader::FindAndReplaceShader() :
	wxutil::TransientWindow(_(FINDDLG_WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow()),
    _lastFocusedEntry(nullptr)
{
	// Create all the widgets
	populateWindow();

	Fit();
	CenterOnParent();

    GlobalShaderClipboard().signal_sourceChanged().connect(
        sigc::mem_fun(this, &FindAndReplaceShader::onShaderClipboardChanged));
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

    findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry")->Connect(
        wxEVT_SET_FOCUS, wxFocusEventHandler(FindAndReplaceShader::onEntryFocusChanged), NULL, this);
    findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry")->Connect(
        wxEVT_SET_FOCUS, wxFocusEventHandler(FindAndReplaceShader::onEntryFocusChanged), NULL, this);

	findNamedObject<wxButton>(this, "FindReplaceDialogFindSelectButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChooseFind), NULL, this);
	findNamedObject<wxButton>(this, "FindReplaceDialogReplaceSelectButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChooseReplace), NULL, this);

    findNamedObject<wxButton>(this, "FindReplaceDialogFindPickButton")->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChoosePick), NULL, this);
    findNamedObject<wxButton>(this, "FindReplaceDialogReplacePickButton")->Connect(
        wxEVT_BUTTON, wxCommandEventHandler(FindAndReplaceShader::onChoosePick), NULL, this);

    findNamedObject<wxButton>(this, "FindReplaceDialogFindPickButton")->SetToolTip(getPickHelpText());
    findNamedObject<wxButton>(this, "FindReplaceDialogReplacePickButton")->SetToolTip(getPickHelpText());

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
	status->SetLabel(fmt::format(_(COUNT_TEXT), replaced));
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

void FindAndReplaceShader::onChoosePick(wxCommandEvent& ev)
{
    // Show the dialog the first time the user clicks this button
    if (!registry::getValue<bool>(RKEY_PICK_HINT_SHOWN, false))
    {
        wxutil::Messagebox::Show(_("Picking Texures"), getPickHelpText(),
            IDialog::MessageType::MESSAGE_CONFIRM, this);

        registry::setValue(RKEY_PICK_HINT_SHOWN, true);
    }

    wxButton* pickFindButton = findNamedObject<wxButton>(this, "FindReplaceDialogFindPickButton");
    wxButton* pickReplaceButton = findNamedObject<wxButton>(this, "FindReplaceDialogReplacePickButton");

    if (ev.GetEventObject() == pickFindButton)
    {
        pickFindButton->SetBackgroundColour(wxColour(220, 0, 0));
        pickReplaceButton->SetBackgroundColour(wxNullColour);

        _lastFocusedEntry = findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry");
    }
    else if (ev.GetEventObject() == pickReplaceButton)
    {
        pickFindButton->SetBackgroundColour(wxNullColour);
        pickReplaceButton->SetBackgroundColour(wxColour(220, 0, 0));

        _lastFocusedEntry = findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry");
    }
}

void FindAndReplaceShader::onClose(wxCommandEvent& ev)
{
    Close();
}

void FindAndReplaceShader::onEntryChanged(wxCommandEvent& ev)
{
	findNamedObject<wxStaticText>(this, "FindReplaceDialogStatusLabel")->SetLabel("");
}

void FindAndReplaceShader::onEntryFocusChanged(wxFocusEvent& ev)
{
    _lastFocusedEntry = wxDynamicCast(ev.GetEventObject(), wxTextCtrl);

    wxButton* pickFindButton = findNamedObject<wxButton>(this, "FindReplaceDialogFindPickButton");
    wxButton* pickReplaceButton = findNamedObject<wxButton>(this, "FindReplaceDialogReplacePickButton");

    if (_lastFocusedEntry == findNamedObject<wxTextCtrl>(this, "FindReplaceDialogFindEntry"))
    {
        pickFindButton->SetBackgroundColour(wxColour(220, 0, 0));
        pickReplaceButton->SetBackgroundColour(wxNullColour);
    }
    else if (_lastFocusedEntry == findNamedObject<wxTextCtrl>(this, "FindReplaceDialogReplaceEntry"))
    {
        pickFindButton->SetBackgroundColour(wxNullColour);
        pickReplaceButton->SetBackgroundColour(wxColour(220, 0, 0));
    }

    ev.Skip();
}

void FindAndReplaceShader::onShaderClipboardChanged()
{
    if (_lastFocusedEntry)
    {
        _lastFocusedEntry->SetValue(GlobalShaderClipboard().getSource().getShader());
    }
}

std::string FindAndReplaceShader::getPickHelpText()
{
    // Find the pick texture mouse tool to get an accurate help text
    IMouseToolGroup& camGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

    MouseToolPtr tool = camGroup.getMouseToolByName(PickShaderTool::NAME());

    unsigned int mapping = camGroup.getMappingForTool(tool);

    std::string bindText = wxutil::Modifier::GetModifierString(mapping);
    bindText += !bindText.empty() ? "-" : "";
    bindText += wxutil::MouseButton::GetButtonString(mapping);

    return fmt::format(_(PICK_TEXTURE_HINT), bindText);
}

void FindAndReplaceShader::ShowDialog(const cmd::ArgumentList& args)
{
	// Just instantiate a new dialog, this enters a main loop
	FindAndReplaceShader* dialog = new FindAndReplaceShader;

	dialog->Show();
}

} // namespace ui
