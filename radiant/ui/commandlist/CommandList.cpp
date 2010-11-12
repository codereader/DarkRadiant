#include "CommandList.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/treeview.h>
#include <gtkmm/stock.h>

#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"

#include "CommandListPopulator.h"
#include "ShortcutChooser.h"

namespace ui
{
	namespace
	{
		const int CMDLISTDLG_DEFAULT_SIZE_X = 550;
	    const int CMDLISTDLG_DEFAULT_SIZE_Y = 400;

	    const char* const CMDLISTDLG_WINDOW_TITLE = N_("Shortcut List");
	}

CommandList::CommandList() :
	gtkutil::BlockingTransientWindow(_(CMDLISTDLG_WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_default_size(CMDLISTDLG_DEFAULT_SIZE_X, CMDLISTDLG_DEFAULT_SIZE_Y);

	// Create all the widgets
	populateWindow();
}

void CommandList::reloadList()
{
	_listStore->clear();

	// Instantiate the visitor class with the target list store
	CommandListPopulator populator(_listStore, _columns);

	// Cycle through all the events and create the according list items
	GlobalEventManager().foreachEvent(populator);
}

void CommandList::populateWindow()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 12));
	add(*hbox);

	{
		// Create a new liststore item and define its columns
		_listStore = Gtk::ListStore::create(_columns);

		_treeView = Gtk::manage(new Gtk::TreeView(_listStore));
		_treeView->append_column(_("Command"), _columns.command);
		_treeView->append_column(_("Key"), _columns.key);

		// Connect the mouseclick event to catch the double clicks
		_treeView->add_events(Gdk::BUTTON_PRESS_MASK);
		_treeView->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &CommandList::callbackViewButtonPress));

		// Load the list items into the treeview
		reloadList();

		// Pack this treeview into a scrolled window and show it
		Gtk::ScrolledWindow* scrolled = Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));

		// Set the sorting column
		Gtk::TreeViewColumn* cmdColumn = _treeView->get_column(0);
		cmdColumn->set_sort_column(_columns.command);

		// Pack the scrolled window into the hbox
		hbox->pack_start(*scrolled, true, true, 0);
	}

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	hbox->pack_start(*vbox, false, false, 0);

	// Create the close button
	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	vbox->pack_end(*closeButton, false, false, 0);
	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandList::callbackClose));
	closeButton->set_size_request(80, -1);

	// Create the assign shortcut button
	Gtk::Button* assignButton = Gtk::manage(new Gtk::Button(Gtk::Stock::EDIT));
	vbox->pack_end(*assignButton, false, false, 0);
	assignButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandList::callbackAssign));

	// Create the clear shortcut button
	Gtk::Button* clearButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
	vbox->pack_end(*clearButton, false, false, 0);
	clearButton->signal_clicked().connect(sigc::mem_fun(*this, &CommandList::callbackClear));
}

std::string CommandList::getSelectedCommand()
{
	Gtk::TreeModel::iterator iter = _treeView->get_selection()->get_selected();

	if (iter)
	{
		const std::string commandName = iter->get_value(_columns.command);

		IEventPtr ev = GlobalEventManager().findEvent(commandName);

		// Double check, if the command exists
		if (ev != NULL)
		{
			return commandName;
		}
	}

	return "";
}

void CommandList::assignShortcut()
{
	std::string command = getSelectedCommand();

	if (command.empty()) return;

	IEventPtr ev = GlobalEventManager().findEvent(command);

	// Instantiate the helper dialog
	ShortcutChooser chooser(_("Enter new Shortcut"), getRefPtr(), command);

	ShortcutChooser::Result result = chooser.run();

	if (result == ShortcutChooser::RESULT_OK)
	{
		// The chooser returned OK, update the list
		reloadList();
	}
}

void CommandList::callbackAssign()
{
	assignShortcut();
}

void CommandList::callbackViewButtonPress(GdkEventButton* ev)
{
	if (ev->type == GDK_2BUTTON_PRESS)
	{
		assignShortcut();
	}
}

void CommandList::callbackClear()
{
	const std::string commandName = getSelectedCommand();

	if (!commandName.empty())
	{
		// Disconnect the event and update the list
		GlobalEventManager().disconnectAccelerator(commandName);
		reloadList();
	}
}

void CommandList::callbackClose()
{
	destroy();

	// Reload all the accelerators
	GlobalUIManager().getMenuManager().updateAccelerators();
}

void CommandList::showDialog(const cmd::ArgumentList& args)
{
	CommandList dialog;
	dialog.show();
}

} // namespace ui
