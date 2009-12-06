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
private:
	// The unique ID of this dialog
	const std::size_t _id;

	// The owning manager
	DialogManager& _owner;

	IDialog::Result _result;

	// The dialog type
	IDialog::Type _type;

	// Packing container, direct child of the GtkWindow
	GtkWidget* _vbox;
	GtkWidget* _buttonHBox;

	// Whether all widgets have been created
	bool _constructed;

public:
	Dialog(std::size_t id, DialogManager& owner, const std::string& title, IDialog::Type type);

	std::size_t getId() const;

	virtual void setTitle(const std::string& title);

	virtual void setDialogType(IDialog::Type type);

	// Enter the main loop
	virtual Result run();

	// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
	// calling any methods on this object results in undefined behavior.
	virtual void destroy();

private:
	// Used for lazy construction of this dialog's widgets, called right before blocking
	void construct();

	void createButtons();

	// GTK Callbacks
	static void onPositive(GtkWidget* widget, Dialog* self);
	static void onNegative(GtkWidget* widget, Dialog* self);
};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace ui

#endif /* _UI_DIALOG_H_ */
