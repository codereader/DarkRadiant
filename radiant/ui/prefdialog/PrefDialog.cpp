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

void PrefDialog::createDialog()
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

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
		PrefPage* pageWidget = new PrefPage(_notebook, page);

		// Remember this page in our mapping
		const std::string& pagePath = page.getPath();

		_pages[pagePath] = pageWidget;

		std::vector<std::string> parts;
		boost::algorithm::split(parts, pagePath, boost::algorithm::is_any_of("/"));

		if (parts.size() > 1)
		{
			parts.pop_back();
			std::string parentPath = boost::algorithm::join(parts, "/");
			
			PageMap::const_iterator parent = _pages.find(parentPath);

			if (parent != _pages.end())
			{
				// Find the index of the parent page to perform the insert
				int pos = _notebook->FindPage(parent->second);
				_notebook->InsertSubPage(pos, pageWidget, page.getName());
			}
			else
			{
				rError() << "Cannot insert page, unable to find parent path: " << parentPath << std::endl;
			}
		}
		else
		{
			// Top-level page
			// Append the panel as new page to the notebook
			_notebook->AddPage(pageWidget, page.getName());
		}
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
	// Destroy the window and all child widgets along with it
	if (_dialog != nullptr)
	{
		_dialog->Destroy();
		_dialog = nullptr;
		_notebook = nullptr;
	}

	_pages.clear();
}

void PrefDialog::doShowModal(const std::string& requestedPage)
{
	createDialog();

	// Reset all values to the ones found in the registry
	for (const PageMap::value_type& p : _pages)
	{
		p.second->resetValues();
	}

	// Trigger a resize of the treebook's TreeCtrl, do this by expanding all nodes 
	// (one would be enough, but we want to show the whole tree anyway)
	for (std::size_t page = 0; page < _notebook->GetPageCount(); ++page)
	{
		_notebook->ExpandNode(page, true);
	}

	_dialog->FitToScreen(0.5f, 0.5f);

	// Is there a specific page display request?
	if (!requestedPage.empty())
	{
		showPage(requestedPage);
	}

	if (_dialog->ShowModal() == wxID_OK)
	{
		// Tell all pages to flush their buffer
		for (const PageMap::value_type& p : _pages)
		{ 
			p.second->saveChanges(); 
		}

		// greebo: Check if the mainframe module is already "existing". It might be
		// uninitialised if this dialog is shown during DarkRadiant startup
		if (module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME))
		{
			GlobalMainFrame().updateAllWindows();
		}
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

	PageMap::const_iterator found = _pages.find(path);

	if (found != _pages.end())
	{
		// Find the page number
		int pagenum = _notebook->FindPage(found->second);

		// make it active
		_notebook->SetSelection(pagenum);
	}
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
