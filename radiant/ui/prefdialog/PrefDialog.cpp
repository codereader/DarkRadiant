#include "PrefDialog.h"

#include "ui/imainframe.h"
#include "itextstream.h"
#include "ipreferencesystem.h"

#include "i18n.h"

#include "wxutil/dialog/DialogBase.h"

#include <wx/sizer.h>
#include <wx/listbook.h>
#include <wx/treectrl.h>
#include "string/split.h"
#include "string/join.h"
#include "string/predicate.h"

namespace ui
{

using NotebookType = wxListbook;

PrefDialog::PrefDialog(wxWindow* parent)
: DialogBase(_("DarkRadiant Preferences"), parent)
{
    wxBoxSizer* mainVbox = new wxBoxSizer(wxVERTICAL);

    // Notebook widget (shows tree of pages and the space for each page to be shown)
    _notebook = new NotebookType(this, wxID_ANY);
    mainVbox->Add(_notebook, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 12);

    // Button box
    mainVbox->AddSpacer(6);
    mainVbox->Add(
        CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM, 12
    );

    // Create the page widgets
    createPages();
    SetSizerAndFit(mainVbox);
}

void PrefDialog::createPages()
{
    // Now create all pages
    GlobalPreferenceSystem().foreachPage([&](IPreferencePage& page)
    {
        // Create a page responsible for this settings::PreferencePage
        PrefPage* pageWidget = new PrefPage(_notebook, page);

        // Remember this page in our mapping
        const std::string& pagePath = page.getPath();
        _pages[pagePath] = pageWidget;

        // Top-level page
        // Append the panel as new page to the notebook
        _notebook->AddPage(pageWidget, page.getName());
    });
}

void PrefDialog::showModal(const std::string& requestedPage)
{
	// Reset all values to the ones found in the registry
	for (const PageMap::value_type& p : _pages)
	{
		p.second->resetValues();
	}

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

void PrefDialog::showPage(const std::string& path)
{
    if (!_notebook)
    {
        rError() << "Can't show requested page, as the wxNotebook is null" << std::endl;
        return;
    }

    if (PageMap::const_iterator found = _pages.find(path); found != _pages.end())
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

} // namespace ui
