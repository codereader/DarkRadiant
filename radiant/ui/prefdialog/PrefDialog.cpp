#include "PrefDialog.h"

#include "imainframe.h"
#include "itextstream.h"

#include "i18n.h"

#include "wxutil/dialog/DialogBase.h"

#include <wx/sizer.h>
#include <wx/treebook.h>
#include <boost/algorithm/string/join.hpp>

#include "settings/PreferenceSystem.h"
#include "settings/PreferencePage.h"

namespace ui
{

PrefDialog::PrefDialog() :
	_dialog(nullptr),
	_notebook(nullptr)
{
	// Register this instance with GlobalRadiant() at once
	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*this, &PrefDialog::onRadiantShutdown)
    );
}

void PrefDialog::createDialog(wxWindow* parent)
{
	assert(_dialog == nullptr); // destroy old dialog first please

	_dialog = new wxutil::DialogBase(_("DarkRadiant Preferences"), parent);
	_dialog->SetSizer(new wxBoxSizer(wxVERTICAL));
	_dialog->SetMinClientSize(wxSize(640, -1));

	// 12-pixel spacer
	_mainVbox = new wxBoxSizer(wxVERTICAL);
	_dialog->GetSizer()->Add(_mainVbox, 1, wxEXPAND | wxALL, 12);

	_mainVbox->Add(_dialog->CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	// Create the notebook and page widgets
	createTreebook();
}

void PrefDialog::createTreebook()
{
	assert(_dialog); // need a valid dialog

	_notebook = new wxTreebook(_dialog, wxID_ANY);
	_mainVbox->Prepend(_notebook, 1, wxEXPAND);

	// Prevent the tree control from shrinking too much
	_notebook->GetTreeCtrl()->SetMinClientSize(wxSize(200, -1));

	// Now create all pages
	GetPreferenceSystem().foreachPage([&](settings::PreferencePage& page)
	{
		// Create a page responsible for this settings::PreferencePage
		// TODO
#if 0
		wxWindow* pageWidget = page.createWidget(_notebook);

		std::vector<std::string> parts;
		boost::algorithm::split(parts, page.getPath(), boost::algorithm::is_any_of("/"));

		if (parts.size() > 1)
		{
			parts.pop_back();
			std::string parentPath = boost::algorithm::join(parts, "/");
			PrefPagePtr parentPage = _root->createOrFindPage(parentPath);

			if (parentPage)
			{
				// Find the index of the parent page to perform the insert
				int pos = _notebook->FindPage(parentPage->getWidget());
				_notebook->InsertSubPage(pos, pageWidget, page.getName());
			}
		}
		else
		{
			// Top-level page
			// Append the panel as new page to the notebook
			_notebook->AddPage(pageWidget, page.getName());
		}
#endif
	});
}

void PrefDialog::onRadiantShutdown()
{
	rMessage() << "PrefDialog shutting down." << std::endl;

	if (_dialog != nullptr && _dialog->IsShownOnScreen())
	{
		_dialog->Hide();
	}

	// Destroy the wxWidgets elements
	destroyDialog();

	InstancePtr().reset();
}

void PrefDialog::destroyDialog()
{
	// Destroy the notebook as well
	if (_notebook)
	{
		_notebook->Destroy();
		_notebook = nullptr;
	}

	// Destroy the window and let it go
	if (_dialog != nullptr)
	{
		_dialog->Destroy();
		_dialog = nullptr;
	}
}

void PrefDialog::doShowModal(const std::string& requestedPage)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
	{
		createDialog(GlobalMainFrame().getWxTopLevelWindow());
	}
	else
	{
		createDialog(nullptr);
	}

	// Trigger a resize of the treebook's TreeCtrl, do this by expanding all nodes 
	// (one would be enough, but we want to show the whole tree anyway)
	for (std::size_t page = 0; page < _notebook->GetPageCount(); ++page)
	{
		_notebook->ExpandNode(page, true);
	}

	_dialog->FitToScreen(0.5f, 0.5f);

	// Discard any changes we got earlier 
	// TODO
#if 0
	_root->foreachPage([&] (PrefPage& page)
	{
		page.discardChanges();
	});
#endif

	// Is there a specific page display request?
	if (!requestedPage.empty())
	{
		showPage(requestedPage);
	}

	if (_dialog->ShowModal() == wxID_OK)
	{
#if 0
		// TODO
		// Tell all pages to flush their buffer
		_root->foreachPage([&] (PrefPage& page) 
		{ 
			page.saveChanges(); 
		});
#endif

		// greebo: Check if the mainframe module is already "existing". It might be
		// uninitialised if this dialog is shown during DarkRadiant startup
		if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
		{
			GlobalMainFrame().updateAllWindows();
		}
	}
	else
	{
#if 0
		// TODO
		// Discard all changes
		_root->foreachPage([&] (PrefPage& page)
		{ 
			page.discardChanges(); 
		});
#endif
	}

	// Clear the dialog once we're done
	destroyDialog();
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

	if (!instancePtr)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new PrefDialog);
	}

	return *instancePtr;
}

void PrefDialog::showPage(const std::string& path)
{
	if (!_notebook)
	{
		rError() << "Can't show requested page, as the wxNotebook is null" << std::endl;
		return;
	}

#if 0
	// TODO
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
#endif
}

void PrefDialog::ShowModal(const std::string& path)
{
	if (Instance()._dialog == nullptr || !Instance()._dialog->IsShownOnScreen())
	{
		Instance().doShowModal(path);
	}
}

void PrefDialog::ShowProjectSettings(const cmd::ArgumentList& args)
{
	ShowModal(_("Game"));
}

} // namespace ui
