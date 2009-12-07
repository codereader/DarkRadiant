#ifndef _UI_DIALOG_H_
#define _UI_DIALOG_H_

#include "idialogmanager.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui
{

class DialogManager;

class Dialog :
	public IDialog,
	public gtkutil::BlockingTransientWindow
{
protected:
	// The unique ID of this dialog
	const std::size_t _id;

	// The owning manager
	DialogManager& _owner;

	IDialog::Result _result;

	// Packing container, direct child of the GtkWindow
	GtkWidget* _vbox;

	// Whether all widgets have been created
	bool _constructed;

public:
	Dialog(std::size_t id, DialogManager& owner, const std::string& title);

	std::size_t getId() const;

	virtual void setTitle(const std::string& title);

	// Enter the main loop
	virtual Result run();
	virtual Result runAndDestroy();

	// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
	// calling any methods on this object results in undefined behavior.
	virtual void destroy();

protected:
	// Constructs the dialog (is invoked right before entering the main loop)
	virtual void construct();

	virtual GtkWidget* createButtons();

	// GTK Callbacks
	static void onOK(GtkWidget* widget, Dialog* self);
	static void onCancel(GtkWidget* widget, Dialog* self);
};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace ui

#endif /* _UI_DIALOG_H_ */
