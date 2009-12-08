#include "Dialog.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>

#include "itextstream.h"

#include "DialogElements.h"

namespace gtkutil
{

Dialog::Dialog(const std::string& title, GtkWindow* parent) :
	BlockingTransientWindow(title, parent),
	_result(RESULT_CANCELLED),
	_vbox(gtk_vbox_new(FALSE, 6)),
	_elementsTable(gtk_table_new(1,1,FALSE)),
	_constructed(false),
	_highestUsedHandle(0)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	gtk_container_add(GTK_CONTAINER(getWindow()), _vbox);

	gtk_table_set_col_spacings(GTK_TABLE(_elementsTable), 12);
    gtk_table_set_row_spacings(GTK_TABLE(_elementsTable), 6);

	gtk_box_pack_start(GTK_BOX(_vbox), _elementsTable, TRUE, TRUE, 0);
}

void Dialog::setTitle(const std::string& title)
{
	// Dispatch this call to the base class
	BlockingTransientWindow::setTitle(title);
}

ui::IDialog::Handle Dialog::addElement(const DialogElementPtr& element)
{
	// Acquire a new handle
	Handle handle = ++_highestUsedHandle;
	
	// Store this element in the map
	_elements[handle] = element;

	guint numRows = static_cast<guint>(_elements.size());

	// Push the widgets into the dialog, resize the table to fit
	gtk_table_resize(GTK_TABLE(_elementsTable), numRows, 2);

	// The label (first column)
	gtk_table_attach(
		GTK_TABLE(_elementsTable), element->getLabel(),
		0, 1, numRows-1, numRows,
		GTK_FILL, (GtkAttachOptions)0, 0, 0
	);

	// The edit widgets (second column)
	gtk_table_attach_defaults(
		GTK_TABLE(_elementsTable), element->getWidget(),
		1, 2, numRows-1, numRows // index starts with 1, hence the -1
	);

	return handle;
}

ui::IDialog::Handle Dialog::addLabel(const std::string& text)
{
	return 0;
}

ui::IDialog::Handle Dialog::addComboBox(const std::string& label, const ComboBoxOptions& options)
{
	return addElement(DialogComboBoxPtr(new DialogComboBox(label, options)));
}

ui::IDialog::Handle Dialog::addEntryBox(const std::string& label)
{
	return 0;
}

ui::IDialog::Handle Dialog::addPathEntry(const std::string& label, bool foldersOnly)
{
	return 0;
}

ui::IDialog::Handle Dialog::addSpinButton(const std::string& label, double min, double max, double step)
{
	return 0;
}

ui::IDialog::Handle Dialog::addCheckbox(const std::string& label)
{
	return 0;
}

void Dialog::setElementValue(const ui::IDialog::Handle& handle, const std::string& value)
{

}

std::string Dialog::getElementValue(const ui::IDialog::Handle& handle)
{
	return "";
}

ui::IDialog::Result Dialog::run()
{
	if (!_constructed)
	{
		_constructed = true;

		// Call the virtual method, gives subclasses a chance to do their stuff
		construct();
	}

	// Show the dialog (enters gtk_main() and blocks)
	show();

	return _result;
}

void Dialog::construct()
{
	// Pack the buttons
	gtk_box_pack_end(GTK_BOX(_vbox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* Dialog::createButtons()
{
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOK), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, FALSE, FALSE, 0);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, FALSE, FALSE, 0);

	return buttonHBox;
}

void Dialog::onCancel(GtkWidget* widget, Dialog* self)
{
	self->_result = ui::IDialog::RESULT_CANCELLED;
	self->hide(); // breaks gtk_main()
}

void Dialog::onOK(GtkWidget* widget, Dialog* self)
{
	self->_result = ui::IDialog::RESULT_OK;
	self->hide(); // breaks gtk_main()
}

} // namespace gtkutil
