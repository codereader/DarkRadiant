#include "Dialog.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>

#include "iradiant.h"
#include "DialogManager.h"

namespace ui
{

Dialog::Dialog(std::size_t id, DialogManager& owner) :
	gtkutil::BlockingTransientWindow("DarkRadiant", GlobalRadiant().getMainWindow()),
	_id(id),
	_owner(owner),
	_vbox(gtk_vbox_new(FALSE, 6))
{
	// Create the buttons and pack them into the window
	gtk_box_pack_end(GTK_BOX(_vbox), createButtons(), FALSE, FALSE, 0);
}

std::size_t Dialog::getId() const
{
	return _id;
}

void Dialog::setTitle(const std::string& title)
{
	// Dispatch this call to the base class
	gtkutil::BlockingTransientWindow::setTitle(title);
}

IDialog::Result Dialog::run()
{
	// Show the dialog (enters gtk_main() and blocks)
	show();

	// Check the result

	return IDialog::RESULT_CANCELLED;
}

// Frees this dialog and all its allocated resources.  Once a dialog as been destroyed, 
// calling any methods on this object results in undefined behavior.
void Dialog::destroy()
{
	// Destroy this window
	BlockingTransientWindow::destroy();

	// Nofity the manager, this will clear ourselves as soon as the last reference is gone
	// which might happen right after this call
	_owner.notifyDestroy(_id);

	// Do not call any other member methods after this point
}

GtkWidget* Dialog::createButtons()
{
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOK), this);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);

	gtk_box_pack_end(GTK_BOX(hbox), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), cancelButton, FALSE, FALSE, 0);

	return hbox;
}

void Dialog::onCancel(GtkWidget* widget, Dialog* self)
{
	self->_result = RESULT_CANCELLED;
	self->hide(); // breaks gtk_main()
}

void Dialog::onOK(GtkWidget* widget, Dialog* self)
{
	self->_result = RESULT_OK;
	self->hide(); // breaks gtk_main()
}

} // namespace ui
