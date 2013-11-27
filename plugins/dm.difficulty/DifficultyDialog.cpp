#include "DifficultyDialog.h"

#include "i18n.h"
#include "iundo.h"
#include "imainframe.h"
#include "iscenegraph.h"

#include "gamelib.h"
#include "registry/registry.h"
#include "string/string.h"
#include "gtkutil/RightAlignment.h"

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/notebook.h>

#include <gdk/gdkkeysyms.h>

#include <iostream>

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Difficulty Editor");

	const std::string RKEY_ROOT = "user/ui/difficultyDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

DifficultyDialog::DifficultyDialog() :
	gtkutil::BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	// Load the settings
	_settingsManager.loadSettings();

	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	set_modal(true);

	signal_key_press_event().connect(sigc::mem_fun(*this, &DifficultyDialog::onWindowKeyPress), false);

	// Create the widgets
	populateWindow();

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

void DifficultyDialog::_preHide()
{
	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

void DifficultyDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();
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

		_notebook->append_page(editor.getEditor(), label);
	}
}

void DifficultyDialog::populateWindow()
{
	// Create the overall vbox
	_dialogVBox = Gtk::manage(new Gtk::VBox(false, 12));
	add(*_dialogVBox);

	// Create the notebook and add it to the vbox
	_notebook = Gtk::manage(new Gtk::Notebook);
	_dialogVBox->pack_start(*_notebook, true, true, 0);

	// Create and pack the editors
	createDifficultyEditors();

	// Pack in dialog buttons
	_dialogVBox->pack_start(createButtons(), false, false, 0);
}

// Lower dialog buttons
Gtk::Widget& DifficultyDialog::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 12));

	// Save button
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyDialog::onSave));
	buttonHBox->pack_end(*okButton, true, true, 0);

	// Close Button
	_closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	_closeButton->signal_clicked().connect(sigc::mem_fun(*this, &DifficultyDialog::onClose));
	buttonHBox->pack_end(*_closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*buttonHBox));
}

void DifficultyDialog::save()
{
	// Consistency check can go here

	// Scoped undo object
	UndoableCommand command("editDifficulty");

	// Save the working set to the entity
	_settingsManager.saveSettings();
}

void DifficultyDialog::onSave()
{
	save();
	destroy();
}

void DifficultyDialog::onClose()
{
	destroy();
}

bool DifficultyDialog::onWindowKeyPress(GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape)
	{
		destroy();
		// Catch this keyevent, don't propagate
		return true;
	}

	// Propagate further
	return false;
}

// Static command target
void DifficultyDialog::showDialog(const cmd::ArgumentList& args)
{
	// Construct a new instance, this enters the main loop
	DifficultyDialog _editor;

	// Show the dialog, this enters the gtk main loop
	_editor.show();
}

} // namespace ui
