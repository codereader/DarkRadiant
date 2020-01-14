#include "PrefDialog.h"

#include "imainframe.h"
#include "itextstream.h"

#include "i18n.h"

#include "wxutil/dialog/DialogBase.h"

#include <wx/sizer.h>
#include <wx/treebook.h>
#include <wx/treectrl.h>
#include "string/split.h"
#include "string/join.h"
#include "string/predicate.h"

#include "settings/PreferenceSystem.h"
#include "settings/PreferencePage.h"

namespace ui
{

PrefDialog::PrefDialog(wxWindow* parent) :
	DialogBase(_("DarkRadiant Preferences"), parent),
	_notebook(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));
	SetMinClientSize(wxSize(640, -1));

	// 12-pixel spacer
	wxBoxSizer* mainVbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(mainVbox, 1, wxEXPAND | wxALL, 12);

	mainVbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);
	
	_notebook = new wxTreebook(this, wxID_ANY);
	
	// Prevent the tree control from shrinking too much
	_notebook->GetTreeCtrl()->SetMinClientSize(wxSize(200, -1));

	mainVbox->Prepend(_notebook, 1, wxEXPAND);

	// Create the page widgets
	createPages();
}

void PrefDialog::createPages()
{
	// Now create all pages
	GetPreferenceSystem().foreachPage([&](settings::PreferencePage& page)
	{
		// Create a page responsible for this settings::PreferencePage
		PrefPage* pageWidget = new PrefPage(_notebook, page);

		// Remember this page in our mapping
		const std::string& pagePath = page.getPath();

		_pages[pagePath] = pageWidget;

		std::vector<std::string> parts;
		string::split(parts, pagePath, "/");

		if (parts.size() > 1)
		{
			parts.pop_back();
			std::string parentPath = string::join(parts, "/");
			
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

void PrefDialog::showModal(const std::string& requestedPage)
{
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

	FitToScreen(0.7f, 0.5f);

	// Is there a specific page display request?
	if (!requestedPage.empty())
	{
		showPage(requestedPage);
	}
	else
	{
		// To prevent starting up with the "Game" node selected, 
		// select the first page below the Settings path
		for (const PageMap::value_type& p : _pages)
		{
			if (string::starts_with(p.first, _("Settings/")))
			{
				showPage(p.first);
				break;
			}
		}
	}

	if (ShowModal() == wxID_OK)
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
}

void PrefDialog::ShowPrefDialog(const cmd::ArgumentList& args)
{
	ShowDialog();
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

void PrefDialog::ShowDialog(const std::string& path)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	PrefDialog* dialog = new PrefDialog(parent);

	dialog->showModal(path);
	dialog->Destroy();
}

void PrefDialog::ShowProjectSettings(const cmd::ArgumentList& args)
{
	ShowDialog(_("Game"));
}

} // namespace ui
