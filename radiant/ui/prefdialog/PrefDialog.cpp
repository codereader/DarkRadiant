#include "PrefDialog.h"

#include "imainframe.h"

#include "i18n.h"

#include "PrefPageWalkers.h"

#include "ui/splash/Splash.h"

#include <wx/sizer.h>
#include <wx/treebook.h>
#include <wx/button.h>

namespace ui
{

PrefDialog::PrefDialog() :
	_dialog(new wxutil::DialogBase(_("DarkRadiant Preferences"), NULL)),
	_notebook(NULL),
	_isModal(false)
{
	// Create a treestore with a name and a pointer
	_prefTree = NULL; //Gtk::TreeStore::create(_treeColumns);

	// Create all the widgets
	populateWindow();

	// Create the root element with the Notebook and Connector references
	_root = PrefPagePtr(new PrefPage("", "", _notebook));
}

void PrefDialog::populateWindow()
{
	_dialog->SetSizer(new wxBoxSizer(wxVERTICAL));
	_mainPanel = new wxPanel(_dialog, wxID_ANY);
	_dialog->GetSizer()->Add(_mainPanel, 1, wxEXPAND);

	_mainPanel->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	//_notebook = new wxTreebook(this, wxID_ANY);

	//GetSizer()->Add(_notebook, 1, wxEXPAND | wxALL, 12);
	_mainPanel->GetSizer()->Add(new wxButton(_mainPanel, wxID_OK), 1, wxEXPAND);
	/*GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, 
		wxALIGN_RIGHT | wxBOTTOM | wxLEFT | wxRIGHT, 12);*/


#if 0
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
#endif
}

void PrefDialog::updateTreeStore()
{
#if 0
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
#endif
}

PrefPagePtr PrefDialog::createOrFindPage(const std::string& path)
{
	// Pass the call to the root page
	return _root->createOrFindPage(path);
}

void PrefDialog::onRadiantShutdown()
{
#if 0
	hide();

	// Destroy the singleton
	InstancePtr().reset();
#endif
}

void PrefDialog::_preShow()
{
#if 0
	// Call base class
	DialogBase::_preShow();

	// Update window parent on show
	if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
	{
		setParentWindow(GlobalMainFrame().getTopLevelWindow());
	}
	// We're still in module initialisation phase, get the splash instance
	else if (Splash::isVisible())
	{
		// wxTODO set_transient_for(Splash::Instance());
	}

	// Discard any changes we got earlier
	_root->foreachPage([&] (PrefPage& page)
	{
		page.discardChanges();
	});
#endif
}

int PrefDialog::ShowModal()
{
	// greebo: Unfortunate hack, move the main panel over to a newly created 
	// window each time we enter the dialog. I tried to use the one which is 
	// created during radiant startup, but it wouldn't show up anymore once 
	// the main app is initialised. Not sure if this is necessary in Linux.
	wxutil::DialogBase* newDialog = new wxutil::DialogBase(_("DarkRadiant Preferences"), NULL);
	newDialog->SetSizer(new wxBoxSizer(wxVERTICAL));
	
	_mainPanel->Reparent(newDialog);
	newDialog->GetSizer()->Add(_mainPanel, 1, wxEXPAND);

	// Destroy the old dialog window and use the new one
	if (_dialog != NULL)
	{
		_dialog->Destroy();
		_dialog = NULL;
	}

	_dialog = newDialog;
	_dialog->FitToScreen(0.7f, 0.6f);

	// Discard any changes we got earlier
	_root->foreachPage([&] (PrefPage& page)
	{
		page.discardChanges();
	});

	// Is there a specific page display request?
	if (!_requestedPage.empty())
	{
		showPage(_requestedPage);
	}

	return _dialog->ShowModal();
}

#if 0
void PrefDialog::Show()
{
	// Pass the call to the utility methods that save/restore the window position
	if (IsShownOnScreen())
	{
		Hide();
	}
	else
	{
		/*wxutil::DialogBase* base = new wxutil::DialogBase("Test", NULL);

		base->SetSizer(new wxBoxSizer(wxVERTICAL));
		base->GetSizer()->Add(new wxButton(base, wxID_OK), 1, wxEXPAND);

		base->Fit();
		base->CenterOnScreen();
		base->ShowModal();

		base->Destroy();*/

		// Rebuild the tree and expand it
		updateTreeStore();
		// wxTODO _treeView->expand_all();

		// Now show the dialog window again (triggers _preShow())
		Fit();
		// Reposition the modal dialog, it has been reset by the size_request call
		Layout();
		CenterOnScreen();
		ShowModal();

		if (isModal)
		{
			_isModal = true;

			// Resize the window to fit the widgets exactly
			Fit();

			// Reposition the modal dialog, it has been reset by the size_request call
			CenterOnParent();

			// Enter the main loop, quit() is called by the buttons
			// wxTODO Gtk::Main::run();
		}
	}
}
#endif

void PrefDialog::ShowDialog(const cmd::ArgumentList& args)
{
	Instance().ShowModal();
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
#if 0
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
#endif
}

void PrefDialog::showPage(const std::string& path)
{
#if 0
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
#endif
}

void PrefDialog::save()
{
#if 0
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
#endif
}

void PrefDialog::cancel()
{
#if 0
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
#endif
}

void PrefDialog::showModal(const std::string& path)
{
	if (!Instance()._dialog->IsShownOnScreen())
	{
		Instance()._requestedPage = path;
		Instance().ShowModal();
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
#if 0
	// Closing the dialog is equivalent to CANCEL
	cancel();

	PersistentTransientWindow::_onDeleteEvent();
#endif
}

} // namespace ui
