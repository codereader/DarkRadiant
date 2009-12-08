#ifndef _UI_DIALOG_H_
#define _UI_DIALOG_H_

#include "idialogmanager.h"
#include "../window/BlockingTransientWindow.h"
#include <map>

namespace gtkutil
{

class DialogManager;

class DialogElement;
typedef boost::shared_ptr<DialogElement> DialogElementPtr;

class Dialog :
	public ui::IDialog,
	public BlockingTransientWindow
{
protected:
	ui::IDialog::Result _result;

	// Packing container, direct child of the GtkWindow
	GtkWidget* _vbox;

	// The table carrying the elements
	GtkWidget* _elementsTable;

	// Whether all widgets have been created
	bool _constructed;

	// The elements added to this dialog, indexed by handle
	typedef std::map<Handle, DialogElementPtr> ElementMap;
	ElementMap _elements;

	Handle _highestUsedHandle;

public:
	Dialog(const std::string& title, GtkWindow* parent = NULL);

	virtual void setTitle(const std::string& title);

	virtual Handle addLabel(const std::string& text);
	virtual Handle addComboBox(const std::string& label, const ComboBoxOptions& options);
	virtual Handle addEntryBox(const std::string& label);
	virtual Handle addPathEntry(const std::string& label, bool foldersOnly = false);
	virtual Handle addSpinButton(const std::string& label, double min, double max, double step);
	virtual Handle addCheckbox(const std::string& label);

	virtual void setElementValue(const Handle& handle, const std::string& value);
	virtual std::string getElementValue(const Handle& handle);

	// Enter the main loop
	virtual ui::IDialog::Result run();

protected:
	// Constructs the dialog (is invoked right before entering the main loop)
	virtual void construct();

	virtual GtkWidget* createButtons();

	ui::IDialog::Handle addElement(const DialogElementPtr& element);

	// Override TransientWindow behaviour to hide this dialog instead of destroying it
	virtual void _onDeleteEvent();

	// GTK Callbacks
	static void onOK(GtkWidget* widget, Dialog* self);
	static void onCancel(GtkWidget* widget, Dialog* self);
};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace gtkutil

#endif /* _UI_DIALOG_H_ */
