#include "Dialog.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

#include "itextstream.h"

#include "DialogElements.h"
#include "../EntryAbortedException.h"

namespace gtkutil
{

Dialog::Dialog(const std::string& title, const Glib::RefPtr<Gtk::Window>& parent) :
	BlockingTransientWindow(title, parent),
	_result(RESULT_CANCELLED),
	_vbox(Gtk::manage(new Gtk::VBox(false, 24))),
	_elementsTable(NULL),
	_accelGroup(Gtk::AccelGroup::create()),
	_constructed(false),
	_highestUsedHandle(0)
{
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Center the popup when no parent window is specified
	if (!parent)
	{
		set_position(Gtk::WIN_POS_CENTER);
	}

	add(*_vbox);

	add_accel_group(_accelGroup);
}

void Dialog::createAndPackElementsTable()
{
    _elementsTable = Gtk::manage(new Gtk::Table(1, 1, false));

	_elementsTable->set_col_spacings(12);
	_elementsTable->set_row_spacings(6);

	_vbox->pack_start(*_elementsTable, true, true, 0);
}

void Dialog::setTitle(const std::string& title)
{
	// Dispatch this call to the base class
	BlockingTransientWindow::set_title(title);
}

ui::IDialog::Handle Dialog::addElement(const DialogElementPtr& element)
{
    // Create the elements table if necessary
    if (!_elementsTable)
    {
        createAndPackElementsTable();
    }

    g_assert(_elementsTable);

	Gtk::Label* first = element->getLabel();
	Gtk::Widget* second = element->getValueWidget();

	if (first == NULL && second == NULL) return ui::INVALID_HANDLE;

	// At least one of the widgets is non-NULL

	// Acquire a new handle
	Handle handle = ++_highestUsedHandle;

	// Store this element in the map
	_elements[handle] = element;

	guint numRows = static_cast<guint>(_elements.size());

	// Push the widgets into the dialog, resize the table to fit
	_elementsTable->resize(numRows, 2);

	if (first != second)
	{
		// Widgets are not equal, check for NULL-ness
		if (second == NULL)
		{
			// One single widget, spanning over two columns
			_elementsTable->attach(*first, 0, 2, numRows-1, numRows); // index starts with 1, hence the -1
		}
		else if (first == NULL)
		{
			// One single widget, spanning over two columns
			_elementsTable->attach(*second, 0, 2, numRows-1, numRows); // index starts with 1, hence the -1
		}
		else // Both are non-NULL
		{
			// The label (first column)
			_elementsTable->attach(*first, 0, 1, numRows-1, numRows, Gtk::FILL, Gtk::AttachOptions(0));

			// The edit widgets (second column)
			_elementsTable->attach(*second, 1, 2, numRows-1, numRows);
		}
	}
	else // The widgets are the same, non-NULL
	{
		// Make it span over two columns
		_elementsTable->attach(*first, 0, 2, numRows-1, numRows); // index starts with 1, hence the -1
	}

	return handle;
}

ui::IDialog::Handle Dialog::addLabel(const std::string& text)
{
	return addElement(DialogElementPtr(new DialogLabel(text)));
}

ui::IDialog::Handle Dialog::addComboBox(const std::string& label, const ComboBoxOptions& options)
{
	return addElement(DialogComboBoxPtr(new DialogComboBox(label, options)));
}

ui::IDialog::Handle Dialog::addEntryBox(const std::string& label)
{
	return addElement(DialogElementPtr(new DialogEntryBox(label)));
}

ui::IDialog::Handle Dialog::addPathEntry(const std::string& label, bool foldersOnly)
{
	return addElement(DialogElementPtr(new DialogPathEntry(label, foldersOnly)));
}

ui::IDialog::Handle Dialog::addSpinButton(const std::string& label, double min, double max, double step, unsigned int digits)
{
	return addElement(DialogElementPtr(new DialogSpinButton(label, min, max, step, digits)));
}

ui::IDialog::Handle Dialog::addCheckbox(const std::string& label)
{
	return addElement(DialogElementPtr(new DialogCheckBox(label)));
}

void Dialog::setElementValue(const ui::IDialog::Handle& handle, const std::string& value)
{
	ElementMap::iterator i = _elements.find(handle);

	if (i == _elements.end())
	{
		rError() << "Dialog: cannot find element with handle " << handle << std::endl;
		return;
	}

	// Import the data from the string into the widget
	i->second->importFromString(value);
}

std::string Dialog::getElementValue(const ui::IDialog::Handle& handle)
{
	ElementMap::iterator i = _elements.find(handle);

	if (i == _elements.end())
	{
		rError() << "Dialog: cannot find element with handle " << handle << std::endl;
		return "";
	}

	// Export the widget's contents to a string
	return i->second->exportToString();
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
	_vbox->pack_end(createButtons(), false, false, 0);
}

Gtk::Widget& Dialog::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 6));

	// OK
	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &Dialog::onOK));
	mapKeyToButton(GDK_Return, *okButton);

	// CANCEL
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &Dialog::onCancel));
	mapKeyToButton(GDK_Escape, *cancelButton);

	buttonHBox->pack_end(*okButton, false, false, 0);
	buttonHBox->pack_end(*cancelButton, false, false, 0);

	return *buttonHBox;
}

void Dialog::mapKeyToButton(guint key, Gtk::Widget& button)
{
	button.add_accelerator("clicked", _accelGroup, key, Gdk::ModifierType(0), Gtk::AccelFlags(0));
}

void Dialog::_onDeleteEvent()
{
	_result = ui::IDialog::RESULT_CANCELLED;
	hide(); // breaks gtk_main()
}

void Dialog::onCancel()
{
	_result = ui::IDialog::RESULT_CANCELLED;
	hide(); // breaks gtk_main()
}

void Dialog::onOK()
{
	_result = ui::IDialog::RESULT_OK;
	hide(); // breaks gtk_main()
}

std::string Dialog::TextEntryDialog(const std::string& title,
								    const std::string& prompt,
								    const std::string& defaultText,
								    const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	Dialog dialog(title, mainFrame);

	Dialog::Handle entryHandle = dialog.addEntryBox(prompt);

	Dialog::Result result = dialog.run();

	if (result == Dialog::RESULT_OK)
	{
		return dialog.getElementValue(entryHandle);
	}
    else
	{
        throw EntryAbortedException("textEntryDialog(): dialog cancelled");
	}
}

} // namespace gtkutil
