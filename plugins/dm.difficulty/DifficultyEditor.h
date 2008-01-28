#ifndef DIFFICULTY_EDITOR_H_
#define DIFFICULTY_EDITOR_H_

#include "ientity.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui {

class DifficultyEditor;
typedef boost::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

class DifficultyEditor :
	public gtkutil::BlockingTransientWindow
{
	// The overall dialog vbox (used to quickly disable the whole dialog)
	GtkWidget* _dialogVBox;
	
	// The close button to toggle the view
	GtkWidget* _closeButton;
	
	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
public:
	DifficultyEditor();
	
	// Command target to toggle the dialog
	static void showDialog();

private:
	virtual void _preHide();
	virtual void _preShow();
	
	// greebo: Saves the current working set to the entity
	void save();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window
	GtkWidget* createButtons(); 	// Dialog buttons
	
	// Button callbacks
	static void onSave(GtkWidget* button, DifficultyEditor* self);
	static void onClose(GtkWidget* button, DifficultyEditor* self);

	// The keypress handler for catching the keys in the treeview
	static gboolean onWindowKeyPress(
		GtkWidget* dialog, GdkEventKey* event, DifficultyEditor* self);

}; // class DifficultyEditor

} // namespace ui

#endif /*DIFFICULTY_EDITOR_H_*/
