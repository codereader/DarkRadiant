#ifndef _UI_DIALOG_H_
#define _UI_DIALOG_H_

#include "idialogmanager.h"
#include "../window/BlockingTransientWindow.h"

namespace gtkutil
{

class DialogManager;

class Dialog :
	public ui::IDialog,
	public BlockingTransientWindow
{
protected:
	ui::IDialog::Result _result;

	// Packing container, direct child of the GtkWindow
	GtkWidget* _vbox;

	// Whether all widgets have been created
	bool _constructed;

public:
	Dialog(const std::string& title, GtkWindow* parent = NULL);

	virtual void setTitle(const std::string& title);

	// Enter the main loop
	virtual ui::IDialog::Result run();

protected:
	// Constructs the dialog (is invoked right before entering the main loop)
	virtual void construct();

	virtual GtkWidget* createButtons();

	// GTK Callbacks
	static void onOK(GtkWidget* widget, Dialog* self);
	static void onCancel(GtkWidget* widget, Dialog* self);
};
typedef boost::shared_ptr<Dialog> DialogPtr;

} // namespace gtkutil

#endif /* _UI_DIALOG_H_ */
