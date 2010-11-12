#ifndef SREDITOR_H_
#define SREDITOR_H_

#include "ientity.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "iscenegraph.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"

#include "StimTypes.h"
#include "SREntity.h"
#include "CustomStimEditor.h"

namespace Gtk
{
	class VBox;
	class Notebook;
	class Button;
}

namespace ui
{

class StimEditor;
class ResponseEditor;

class StimResponseEditor;
typedef boost::shared_ptr<StimResponseEditor> StimResponseEditorPtr;

class StimResponseEditor :
	public gtkutil::BlockingTransientWindow
{
private:
	// The overall dialog vbox (used to quickly disable the whole dialog)
	Gtk::VBox* _dialogVBox;

	Gtk::Notebook* _notebook;

	int _stimPageNum;
	int _responsePageNum;
	int _customStimPageNum;
	static int _lastShownPage;

	// The close button to toggle the view
	Gtk::Button* _closeButton;

	// The "extended" entity object managing the stims
	SREntityPtr _srEntity;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

	// The entity we're editing
	Entity* _entity;

	// The helper class managing the stims
	StimTypes _stimTypes;

	// The helper classes for editing the stims/responses
	StimEditor* _stimEditor;
	ResponseEditor* _responseEditor;
	CustomStimEditor* _customStimEditor;

public:
	StimResponseEditor();

	// Command target to toggle the dialog
	static void showDialog(const cmd::ArgumentList& args);

private:
	virtual void _preHide();
	virtual void _preShow();

	/** greebo: Saves the current working set to the entity
	 */
	void save();

	/* WIDGET POPULATION */
	void populateWindow(); 			// Main window
	Gtk::Widget& createButtons(); 	// Dialog buttons

	/** greebo: Checks the selection for a single entity.
	 */
	void rescanSelection();

	// Button callbacks
	void onSave();
	void onClose();

	// The keypress handler for catching the keys in the treeview
	bool onWindowKeyPress(GdkEventKey* ev);

}; // class StimResponseEditor

} // namespace ui

#endif /*SREDITOR_H_*/
