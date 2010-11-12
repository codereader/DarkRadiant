#ifndef DIFFICULTY_DIALOG_H_
#define DIFFICULTY_DIALOG_H_

#include "ientity.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include "DifficultyEditor.h"
#include "DifficultySettingsManager.h"

namespace Gtk
{
	class Widget;
	class VBox;
	class Notebook;
	class Button;
}

namespace ui {

class DifficultyDialog;
typedef boost::shared_ptr<DifficultyDialog> DifficultyDialogPtr;

/**
 * greebo: A difficulty dialog is a modal top-level window which provides
 *         views and controls facilitating the editing of difficulty settings.
 *
 * Maintains a certain number of DifficultyEditors which get packed into the
 * notebook tabs.
 */
class DifficultyDialog :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	Gtk::VBox* _dialogVBox;

	Gtk::Notebook* _notebook;

	// The difficulty settings manager
	difficulty::DifficultySettingsManager _settingsManager;

	std::vector<DifficultyEditorPtr> _editors;

	// The close button to toggle the view
	Gtk::Button* _closeButton;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

public:
	DifficultyDialog();

	// Command target to toggle the dialog
	static void showDialog(const cmd::ArgumentList& args);

private:
	virtual void _preHide();
	virtual void _preShow();

	// greebo: Saves the current working set to the entity
	void save();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window
	void createDifficultyEditors();
	Gtk::Widget& createButtons(); 	// Dialog buttons

	// Button callbacks
	void onSave();
	void onClose();

	// The keypress handler for catching the keys in the treeview
	bool onWindowKeyPress(GdkEventKey* ev);

}; // class DifficultyDialog

} // namespace ui

#endif /*DIFFICULTY_DIALOG_H_*/
