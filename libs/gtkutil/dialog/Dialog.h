#ifndef _UI_DIALOG_H_
#define _UI_DIALOG_H_

#include "idialogmanager.h"
#include "../window/BlockingTransientWindow.h"
#include <map>
#include <gtkmm/accelgroup.h>

namespace Gtk
{
	class VBox;
	class Table;
}

namespace gtkutil
{

class DialogManager;

class DialogElement;
typedef boost::shared_ptr<DialogElement> DialogElementPtr;

/**
 * greebo: A customisable Dialog window featuring Ok and Cancel buttons.
 *
 * Dialog elements can be added to the window using the addLabel(),
 * addButton(), add*() methods etc., which are returning a unique Handle.
 *
 * Use the getElementValue() and setElementValue() methods to
 * get and set the values of these dialog elements.
 *
 * Once the run() method is invoked, the Dialog enters a gtk main loop,
 * showing the dialog and blocking the application. Use the result
 * returned by run() to see which action the user has taken.
 */
class Dialog :
	public ui::IDialog,
	public BlockingTransientWindow
{
protected:
	ui::IDialog::Result _result;

	// Packing container, direct child of the GtkWindow
	Gtk::VBox* _vbox;

	// The table carrying the elements
	Gtk::Table* _elementsTable;

	// Keyboard accel group used to map ENTER and ESC to buttons
	Glib::RefPtr<Gtk::AccelGroup> _accelGroup;

	// Whether all widgets have been created
	bool _constructed;

	// The elements added to this dialog, indexed by handle
	typedef std::map<Handle, DialogElementPtr> ElementMap;
	ElementMap _elements;

	Handle _highestUsedHandle;

private:

    void createAndPackElementsTable();

public:
	Dialog(const std::string& title, const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>());

	virtual void setTitle(const std::string& title);

	virtual Handle addLabel(const std::string& text);
	virtual Handle addComboBox(const std::string& label, const ComboBoxOptions& options);
	virtual Handle addEntryBox(const std::string& label);
	virtual Handle addPathEntry(const std::string& label, bool foldersOnly = false);
	virtual Handle addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits);
	virtual Handle addCheckbox(const std::string& label);

	virtual void setElementValue(const Handle& handle, const std::string& value);
	virtual std::string getElementValue(const Handle& handle);

	// Enter the main loop
	virtual ui::IDialog::Result run();

	// Add a custom DialogElement
	ui::IDialog::Handle addElement(const DialogElementPtr& element);

protected:
	// Constructs the dialog (is invoked right before entering the main loop)
	virtual void construct();

	virtual Gtk::Widget& createButtons();

	void mapKeyToButton(guint key, Gtk::Widget& button);

	// Override TransientWindow behaviour to hide this dialog instead of destroying it
	virtual void _onDeleteEvent();

	// gtkmm button Callbacks
	void onOK();
	void onCancel();

public:
	// Static methods to display pre-fabricated dialogs

	/**
	 * Display a text entry dialog with the given title and prompt text. Returns a
	 * std::string with the entered value, or throws EntryAbortedException if the
	 * dialog was cancelled. The text entry will be filled with the given defaultText
	 * at start.
	 */
	static std::string TextEntryDialog(const std::string& title,
									   const std::string& prompt,
									   const std::string& defaultText,
									   const Glib::RefPtr<Gtk::Window>& mainFrame);

};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace gtkutil

#endif /* _UI_DIALOG_H_ */
