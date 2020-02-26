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

        if (settings != NULL)
        {
            _editors.push_back(
                DifficultyEditorPtr(new DifficultyEditor(
                    _notebook, _settingsManager.getDifficultyName(i), settings)
                )
            );
        }
    }

    // A new image list for the notebook tab icons
    _imageList.reset(new wxImageList(16, 16));
    _notebook->SetImageList(_imageList.get());

    // Pack the editors into the notebook
    for (std::size_t i = 0; i < _editors.size(); i++)
    {
        DifficultyEditor& editor = *_editors[i];

        wxWindow* editorWidget = editor.getEditor();
        std::string icon = editor.getNotebookIconName();

        // Load the icon
        int imageId = icon.empty() ? -1 : 
            _imageList->Add(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + icon));
    
        editorWidget->Reparent(_notebook);
        _notebook->AddPage(editorWidget, editor.getNotebookLabel(), false, imageId);
    }
}

void DifficultyDialog::populateWindow()
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Create the notebook and add it to the vbox
    _notebook = new wxNotebook(this, wxID_ANY);
    _notebook->SetMinClientSize(wxSize(800, 400));

    // Create and pack the editors
    createDifficultyEditors();

    GetSizer()->Add(_notebook, 1, wxEXPAND | wxALL, 12);

    wxButton* okButton = new wxButton(this, wxID_OK);
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL);

    okButton->Bind(wxEVT_BUTTON, [&] (wxCommandEvent&) { EndModal(wxID_OK); });
    cancelButton->Bind(wxEVT_BUTTON, [&] (wxCommandEvent&) { EndModal(wxID_CANCEL); });

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(cancelButton, 0, wxRIGHT, 6);
    buttonSizer->Add(okButton, 0, wxRIGHT, 6);

    GetSizer()->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 12);

    Layout();
    Fit();
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
    // Construct a new instance, this enters the main loop
    DifficultyDialog* editor = new DifficultyDialog;

    editor->ShowModal();
    editor->Destroy();
}

} // namespace ui
