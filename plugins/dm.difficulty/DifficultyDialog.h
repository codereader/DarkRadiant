#ifndef DIFFICULTY_DIALOG_H_
#define DIFFICULTY_DIALOG_H_

#include "ientity.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include "DifficultyEditor.h"
#include "DifficultySettingsManager.h"

// Forward decl.
typedef struct _GtkNotebook GtkNotebook;

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
	GtkWidget* _dialogVBox;

	GtkNotebook* _notebook;

	// The difficulty settings manager
	difficulty::DifficultySettingsManager _settingsManager;

	std::vector<DifficultyEditorPtr> _editors;
	
	// The close button to toggle the view
	GtkWidget* _closeButton;
	
	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
public:
	DifficultyDialog();
	
	// Command target to toggle the dialog
	static void showDialog();

private:
	virtual void _preHide();
	virtual void _preShow();
	
	// greebo: Saves the current working set to the entity
	void save();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window
	void createDifficultyEditors();
	GtkWidget* createButtons(); 	// Dialog buttons
	
	// Button callbacks
	static void onSave(GtkWidget* button, DifficultyDialog* self);
	static void onClose(GtkWidget* button, DifficultyDialog* self);

	// The keypress handler for catching the keys in the treeview
	static gboolean onWindowKeyPress(
		GtkWidget* dialog, GdkEventKey* event, DifficultyDialog* self);

}; // class DifficultyDialog

} // namespace ui

#endif /*DIFFICULTY_DIALOG_H_*/
