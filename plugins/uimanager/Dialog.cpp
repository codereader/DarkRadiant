#include "Dialog.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>

#include "itextstream.h"
#include "iradiant.h"
#include "DialogManager.h"

namespace ui
{

Dialog::Dialog(std::size_t id, DialogManager& owner, const std::string& title, IDialog::Type type) :
	gtkutil::BlockingTransientWindow(title, GlobalRadiant().getMainWindow()),
	_id(id),
	_owner(owner),
	_type(type),
	_vbox(gtk_vbox_new(FALSE, 6)),
	_buttonHBox(gtk_hbox_new(FALSE, 6)),
	_constructed(false)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_container_add(GTK_CONTAINER(getWindow()), _vbox);

	// Pack the button hbox into the window, so that its position in the hbox is reserved
	gtk_box_pack_end(GTK_BOX(_vbox), _buttonHBox, FALSE, FALSE, 0);
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

void Dialog::setDialogType(IDialog::Type type)
{
	if (_constructed) 
	{
		globalWarningStream() << "Cannot set type, dialog has already been constructed/shown." << std::endl;
		return;
	}

	_type = type;
}

IDialog::Result Dialog::run()
{
	// Check if we've constructed the dialog already
	if (!_constructed)
	{
		construct();
	}

	// Show the dialog (enters gtk_main() and blocks)
	show();

	return _result;
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

void Dialog::construct()
{
	if (_constructed) return;

	_constructed = true;

	// Create the buttons, according to this dialog's type
	createButtons();
}

void Dialog::createButtons()
{
	if (_type == DIALOG_OK || DIALOG_OK_CANCEL)
	{
		GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
		g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onPositive), this);
		gtk_box_pack_end(GTK_BOX(_buttonHBox), okButton, FALSE, FALSE, 0);

		if (_type == DIALOG_OK_CANCEL)
		{
			GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
			g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onNegative), this);
			gtk_box_pack_end(GTK_BOX(_buttonHBox), cancelButton, FALSE, FALSE, 0);
		}
	}
	else if (_type == DIALOG_YES_NO)
	{
		GtkWidget* yesButton = gtk_button_new_from_stock(GTK_STOCK_YES);
		g_signal_connect(G_OBJECT(yesButton), "clicked", G_CALLBACK(onPositive), this);
		gtk_box_pack_end(GTK_BOX(_buttonHBox), yesButton, FALSE, FALSE, 0);

		GtkWidget* noButton = gtk_button_new_from_stock(GTK_STOCK_NO);
		g_signal_connect(G_OBJECT(noButton), "clicked", G_CALLBACK(onNegative), this);
		gtk_box_pack_end(GTK_BOX(_buttonHBox), noButton, FALSE, FALSE, 0);
	}
	else
	{
		globalErrorStream() << "Invalid Dialog type encountered" << std::endl;
	}
}

void Dialog::onNegative(GtkWidget* widget, Dialog* self)
{
	self->_result = (self->_type == DIALOG_YES_NO) ? RESULT_NO : RESULT_CANCELLED;
	self->hide(); // breaks gtk_main()
}

void Dialog::onPositive(GtkWidget* widget, Dialog* self)
{
	self->_result = (self->_type == DIALOG_YES_NO) ? RESULT_YES : RESULT_OK;
	self->hide(); // breaks gtk_main()
}

} // namespace ui
