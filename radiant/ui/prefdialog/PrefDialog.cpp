#include "PrefDialog.h"

#include "imainframe.h"
#include "itextstream.h"

#include "i18n.h"

#include "wxutil/dialog/DialogBase.h"

#include <wx/sizer.h>
#include <wx/treebook.h>

namespace ui
{

PrefDialog::PrefDialog() :
	_dialog(NULL),
	_notebook(NULL)
{
	// There is no main application window by the time this constructor is called.
	createDialog(NULL);

	// Create the root element with the Notebook and Connector references
	_root = PrefPagePtr(new PrefPage("", _notebook));

	// Register this instance with GlobalRadiant() at once
	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*this, &PrefDialog::onRadiantShutdown)
    );
	GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(*this, &PrefDialog::onRadiantStartup)
    );
}

void PrefDialog::createDialog(wxWindow* parent)
{
	wxutil::DialogBase* newDialog = new wxutil::DialogBase(_("DarkRadiant Preferences"), parent);
	newDialog->SetSizer(new wxBoxSizer(wxVERTICAL));
    newDialog->SetMinClientSize(wxSize(640, -1));

	// 12-pixel spacer
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	newDialog->GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	if (_notebook != NULL)
	{
		_notebook->Reparent(newDialog);
	}
	else
	{
		_notebook = new wxTreebook(newDialog, wxID_ANY);
	}

	vbox->Add(_notebook, 1, wxEXPAND);
	vbox->Add(newDialog->CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	// Destroy the old dialog window and use the new one
	if (_dialog != NULL)
	{
		_dialog->Destroy();
		_dialog = NULL;
	}

	_dialog = newDialog;
}

PrefPagePtr PrefDialog::createOrFindPage(const std::string& path)
{
	// Pass the call to the root page
	return _root->createOrFindPage(path);
}

void PrefDialog::onRadiantStartup()
{
	// greebo: Unfortunate step needed at least in Windows. Creating a modal
	// dialog with a NULL parent (like we are doing) results in the dialog
	// using the currently active top level window as parent, which in our case
	// is the Splash window. The splash window is destroyed and this
	// dialog will be affected. So recreate the dialog window and reparent the
	// notebook to the new instance before the Splash screen goes the way of the dodo.
	createDialog(GlobalMainFrame().getWxTopLevelWindow());
}

void PrefDialog::onRadiantShutdown()
{
	rMessage() << "PrefDialog shutting down." << std::endl;

	if (_dialog->IsShownOnScreen())
	{
		_dialog->Hide();
	}

	// Destroy the window and let it go
	_dialog->Destroy();
	_dialog = NULL;
	_notebook = NULL;

	InstancePtr().reset();
}

void PrefDialog::doShowModal(const std::string& requestedPage)
{
	// Trigger a resize of the treebook's TreeCtrl
	_notebook->ExpandNode(0, true); 

	_dialog->FitToScreen(0.5f, 0.5f);

	// Discard any changes we got earlier
	_root->foreachPage([&] (PrefPage& page)
	{
		page.discardChanges();
	});

	// Is there a specific page display request?
	if (!requestedPage.empty())
	{
		showPage(requestedPage);
	}

	if (_dialog->ShowModal() == wxID_OK)
	{
		// Tell all pages to flush their buffer
		_root->foreachPage([&] (PrefPage& page) 
		{ 
			page.saveChanges(); 
		});

		// greebo: Check if the mainframe module is already "existing". It might be
		// uninitialised if this dialog is shown during DarkRadiant startup
		if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
		{
			GlobalMainFrame().updateAllWindows();
		}
	}
	else
	{
		// Discard all changes
		_root->foreachPage([&] (PrefPage& page)
		{ 
			page.discardChanges(); 
		});
	}
}

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
	}

	return *instancePtr;
}

void PrefDialog::showPage(const std::string& path)
{
	_root->foreachPage([&] (PrefPage& page)
	{
		// Check for a match
		if (page.getPath() == path)
		{
			// Find the page number
			int pagenum = _notebook->FindPage(page.getWidget());

			// make it active
			_notebook->SetSelection(pagenum);
		}
	});
}

void PrefDialog::ShowModal(const std::string& path)
{
	if (!Instance()._dialog->IsShownOnScreen())
	{
		Instance().doShowModal(path);
	}
}

void PrefDialog::ShowProjectSettings(const cmd::ArgumentList& args)
{
	ShowModal(_("Game"));
}

} // namespace ui
