#include "DifficultyDialog.h"

#include "i18n.h"
#include "iundo.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "iscenegraph.h"

#include "gamelib.h"
#include "registry/registry.h"
#include "string/string.h"

#include <iostream>

#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Difficulty Editor");
}

DifficultyDialog::DifficultyDialog() :
    DialogBase(_(WINDOW_TITLE))
{
    // Load the settings
    _settingsManager.loadSettings();

    // Create the widgets
    populateWindow();
}

void DifficultyDialog::createDifficultyEditors()
{
    int numLevels = game::current::getValue<int>(GKEY_DIFFICULTY_LEVELS);

    for (int i = 0; i < numLevels; i++)
    {
        // Acquire the settings object
        difficulty::DifficultySettingsPtr settings = _settingsManager.getSettings(i);
        if (settings)
        {
            // Construct the editor for this difficulty level and add it to our
            // internal list of editors
            std::string diffName = _settingsManager.getDifficultyName(i);
            auto editor = std::make_shared<DifficultyEditor>(_notebook,
                                                             settings);
            _editors.push_back(editor);

            // Insert the editor's widget as a new page in the choicebook
            wxWindow* editorWidget = editor->getWidget();
            editorWidget->Reparent(_notebook);
            _notebook->AddPage(editorWidget, diffName, false);
        }
    }
}

namespace
{
    // Simple dialog for editing a difficulty setting name
    class EditNameDialog: public wxDialog
    {
        wxTextCtrl* _textCtrl = nullptr;

    public:

        // Construct and initialise with parent and initial text to edit
        EditNameDialog(wxWindow* parent, const wxString& initialText)
        : wxDialog(parent, wxID_ANY, _("Difficulty name"))
        {
            wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

            // Add the edit text box
            _textCtrl=  new wxTextCtrl(this, wxID_ANY, initialText);
            mainSizer->Add(_textCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);
            mainSizer->AddSpacer(6);

            // Add the buttons
            wxSizer* buttons = CreateButtonSizer(wxOK | wxCANCEL);
            mainSizer->Add(buttons, 0, wxEXPAND | wxBOTTOM, 12);

            SetSizer(mainSizer);
            Fit();

            // Start with the text selected and focussed to save an extra mouse
            // click
            _textCtrl->SelectAll();
            _textCtrl->SetFocus();
        }

        // Get the result of editing
        std::string editResult() const
        {
            return _textCtrl->GetValue().ToStdString();
        }
    };
}

void DifficultyDialog::populateWindow()
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Create the notebook and add it to the vbox
    _notebook = new wxChoicebook(this, wxID_ANY);
    _notebook->SetMinClientSize(wxSize(800, 400));

    // Add the edit button alongside the dropdown
    wxSizer* choiceSizer = _notebook->GetControlSizer();
    wxButton* editBtn = new wxButton(
        _notebook, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxBU_EXACTFIT | wxBU_NOTEXT
    );
    editBtn->Bind(wxEVT_BUTTON,
                  [&] (wxCommandEvent&) { editCurrentDifficultyName(); });
    editBtn->SetBitmap(wxArtProvider::GetBitmap("darkradiant:edit.png"));
    choiceSizer->Add(editBtn, 0, wxEXPAND);

    // Create and pack the editors
    createDifficultyEditors();

    GetSizer()->Add(_notebook, 1, wxEXPAND | wxALL, 12);

    wxButton* okButton = new wxButton(this, wxID_OK);
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL);

    okButton->Bind(wxEVT_BUTTON, [&] (wxCommandEvent&) { EndModal(wxID_OK); });
    cancelButton->Bind(wxEVT_BUTTON, [&] (wxCommandEvent&) { EndModal(wxID_CANCEL); });

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(cancelButton);
    buttonSizer->AddSpacer(6);
    buttonSizer->Add(okButton);

    GetSizer()->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 12);

    Layout();
    Fit();
}

void DifficultyDialog::editCurrentDifficultyName()
{
    // Initialise an EditNameDialog with the current tab text as the initial
    // name to edit
    int curDiffLevel = _notebook->GetSelection(); // assume tabs numbered from 0
    EditNameDialog dialog(this, _notebook->GetPageText(curDiffLevel));
    if (dialog.ShowModal() == wxID_OK)
    {
        // Successful edit, get the changed name
        std::string newName = dialog.editResult();

        // Change the difficulty name in the map
        _settingsManager.setDifficultyName(curDiffLevel, newName);

        // Change the displayed name in the dialog
        _notebook->SetPageText(curDiffLevel, newName);
    }
}

void DifficultyDialog::save()
{
    // Consistency check can go here

    // Scoped undo object
    UndoableCommand command("editDifficulty");

    // Save the working set to the entity
    _settingsManager.saveSettings();
}

int DifficultyDialog::ShowModal()
{
    int returnCode = DialogBase::ShowModal();

    if (returnCode == wxID_OK)
    {
        save();
    }

    return returnCode;
}

// Static command target
void DifficultyDialog::ShowDialog(const cmd::ArgumentList& args)
{
    // Construct a new instance and enter the main loop
    DifficultyDialog editor;
    editor.ShowModal();
}

} // namespace ui
