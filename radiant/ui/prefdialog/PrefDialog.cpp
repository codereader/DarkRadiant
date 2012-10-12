#include "PrefDialog.h"

#include "imainframe.h"

#include "i18n.h"

#include "gtkutil/TextColumn.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/LeftAlignment.h"
#include "PrefPageWalkers.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/button.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/stock.h>
#include <gtkmm/main.h>

#include "ui/splash/Splash.h"

namespace ui
{

PrefDialog::PrefDialog() :
	PersistentTransientWindow(_("DarkRadiant Preferences"), Glib::RefPtr<Gtk::Window>(), true),
	_isModal(false)
{
	set_modal(true);
	set_position(Gtk::WIN_POS_CENTER);

	// Set the default border width in accordance to the HIG
	set_border_width(8);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Create a treestore with a name and a pointer
	_prefTree = Gtk::TreeStore::create(_treeColumns);

	// Create all the widgets
	populateWindow();

	// Create the root element with the Notebook and Connector references
	_root = PrefPagePtr(new PrefPage("", "", _notebook));

	add(*_overallVBox);
}

void PrefDialog::populateWindow()
{
	// The overall dialog vbox
	_overallVBox = Gtk::manage(new Gtk::VBox(false, 8));

	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 8));

	_treeView = Gtk::manage(new Gtk::TreeView(_prefTree));

	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &PrefDialog::onPrefPageSelect));

	_treeView->set_headers_visible(false);
	_treeView->append_column(_("Category"), _treeColumns.name);

	_treeView->set_size_request(170, -1);

	Gtk::ScrolledWindow* scrolledFrame = Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));
	hbox->pack_start(*scrolledFrame, false, false, 0);

	_notebook = Gtk::manage(new Gtk::Notebook);
	_notebook->set_show_tabs(false);
	hbox->pack_start(*_notebook, true, true, 0);

	// Pack the notebook and the treeview into the overall dialog vbox
	_overallVBox->pack_start(*hbox, true, true, 0);

	// Create the buttons
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 0));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &PrefDialog::onOK));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &PrefDialog::onCancel));

	buttonHBox->pack_end(*okButton, false, false, 0);
	buttonHBox->pack_end(*cancelButton, false, false, 6);

	_overallVBox->pack_start(*buttonHBox, false, false, 0);
}

void PrefDialog::updateTreeStore()
{
	// Clear the tree before populating it
	_prefTree->clear();

	// Instantiate a new populator class
	gtkutil::VFSTreePopulator vfsTreePopulator(_prefTree);

	PrefTreePopulator visitor(vfsTreePopulator, *this);

	// Visit each page with the PrefTreePopulator
	// (which in turn is using the VFSTreePopulator helper)
	_root->foreachPage(visitor);

	// All the GtkTreeIters are available, we should add the data now
	// re-use the visitor, it provides both visit() methods
	vfsTreePopulator.forEachNode(visitor);
}

PrefPagePtr PrefDialog::createOrFindPage(const std::string& path)
{
	// Pass the call to the root page
	return _root->createOrFindPage(path);
}

void PrefDialog::onRadiantShutdown()
{
	hide();

	// Destroy the singleton
	InstancePtr().reset();
}

void PrefDialog::_preShow()
{
	// Call base class
	PersistentTransientWindow::_preShow();

	// Update window parent on show
	if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
	{
		setParentWindow(GlobalMainFrame().getTopLevelWindow());
	}
	// We're still in module initialisation phase, get the splash instance
	else if (Splash::isVisible())
	{
		set_transient_for(Splash::Instance());
	}

	// Discard any changes we got earlier
	_root->foreachPage([&] (PrefPage& page)
	{
		page.discardChanges();
	});
}

void PrefDialog::toggleWindow(bool isModal)
{
	// Pass the call to the utility methods that save/restore the window position
	if (is_visible())
	{
		PersistentTransientWindow::hide();
	}
	else
	{
		// Rebuild the tree and expand it
		updateTreeStore();
		_treeView->expand_all();

		// Now show the dialog window again (triggers _preShow())
		PersistentTransientWindow::show();

		// Is there a specific page display request?
		if (!_requestedPage.empty())
		{
			showPage(_requestedPage);
		}

		if (isModal)
		{
			_isModal = true;

			// Resize the window to fit the widgets exactly
			set_size_request(-1, -1);

			// Reposition the modal dialog, it has been reset by the size_request call
			set_position(Gtk::WIN_POS_CENTER);

			// Enter the main loop, quit() is called by the buttons
			Gtk::Main::run();
		}
	}
}

void PrefDialog::toggle(const cmd::ArgumentList& args)
{
	Instance().toggleWindow();
}

PrefDialogPtr& PrefDialog::InstancePtr()
{
	static PrefDialogPtr _instancePtr;
	return _instancePtr;
}

PrefDialog& PrefDialog::Instance()
{
	PrefDialogPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new PrefDialog);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &PrefDialog::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

void PrefDialog::selectPage()
{
	// Get the widget* pointer from the current selection
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		// Retrieve the widget pointer from the current row
		Gtk::Widget* page = iter->get_value(_treeColumns.pageWidget);

		int pagenum = _notebook->page_num(*page);

		if (_notebook->get_current_page() != pagenum)
		{
			_notebook->set_current_page(pagenum);
		}
	}
}

void PrefDialog::showPage(const std::string& path)
{
	PrefPagePtr page;

	PrefPageFinder finder(path, page);
	_root->foreachPage(finder);

	if (page != NULL)
	{
		// Find the page number
		int pagenum = _notebook->page_num(page->getWidget());

		// make it active
		_notebook->set_current_page(pagenum);
	}
}

void PrefDialog::save()
{
	if (_isModal)
	{
		Gtk::Main::quit();
		_isModal = false;
	}

	// Tell all pages to flush their buffer
	_root->foreachPage([&] (PrefPage& page) 
	{ 
		page.saveChanges(); 
	});

	toggleWindow();
	_requestedPage = "";

	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
	{
		GlobalMainFrame().updateAllWindows();
	}
}

void PrefDialog::cancel()
{
	if (_isModal)
	{
		Gtk::Main::quit();
		_isModal = false;
	}

	// Discard all changes
	_root->foreachPage([&] (PrefPage& page)
	{ 
		page.discardChanges(); 
	});

	toggleWindow();
	_requestedPage = "";
}

void PrefDialog::showModal(const std::string& path)
{
	if (!Instance().is_visible())
	{
		Instance()._requestedPage = path;
		Instance().toggleWindow(true);
	}
}

void PrefDialog::showProjectSettings(const cmd::ArgumentList& args)
{
	showModal(_("Game"));
}

void PrefDialog::onOK()
{
	save();
}

void PrefDialog::onCancel()
{
	cancel();
}

void PrefDialog::onPrefPageSelect()
{
	selectPage();
}

void PrefDialog::_onDeleteEvent()
{
	// Closing the dialog is equivalent to CANCEL
	cancel();

	PersistentTransientWindow::_onDeleteEvent();
}

} // namespace ui
