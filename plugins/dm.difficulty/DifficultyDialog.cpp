#include "DifficultyDialog.h"

#include "i18n.h"
#include "iundo.h"
#include "imainframe.h"
#include "iscenegraph.h"

#include "gamelib.h"
#include "registry/registry.h"
#include "string/string.h"

#include <iostream>

#include <wx/notebook.h>
#include <wx/panel.h>

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
					_settingsManager.getDifficultyName(i), settings)
				)
			);
		}
	}

	for (std::size_t i = 0; i < _editors.size(); i++)
	{
		DifficultyEditor& editor = *_editors[i];

		Gtk::Widget& label = editor.getNotebookLabel();
		// Show the widgets before using them as label, they won't appear otherwise
		label.show_all();

		//_notebook->append_page(editor.getEditor(), label);
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
	GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxALL, 12);

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
