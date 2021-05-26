#include "PointFileChooser.h"

#include "imainframe.h"
#include "imap.h"
#include "icommandsystem.h"
#include "os/fs.h"
#include "command/ExecutionFailure.h"

#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <list>

namespace ui
{

PointFileChooser::PointFileChooser(const wxArrayString& files)
: wxDialog(GlobalMainFrame().getWxTopLevelWindow(), wxID_ANY,
           "Choose pointfile")
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Construct and populate the dropdown list
    auto list = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             files);
    list->SetSelection(0);
    GetSizer()->Add(list, 0, wxEXPAND | wxALL, 12);

    // Add dialog buttons
    auto btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    GetSizer()->Add(btnSizer, 0, wxALIGN_RIGHT | wxALIGN_BOTTOM | wxBOTTOM, 12);

    Fit();
}

void PointFileChooser::chooseAndToggle()
{
    // Determine the list of pointfiles
    std::list<fs::path> pointfiles;
    GlobalMapModule().forEachPointfile([&](const fs::path& p)
                                       { pointfiles.push_back(p); });
    if (pointfiles.empty())
        throw cmd::ExecutionFailure("No pointfiles found for current map.");

    // If there is a choice to make, show the dialog
    if (!GlobalMapModule().isPointTraceVisible() && pointfiles.size() > 1)
    {
        // Construct list of wxString filenames
        wxArrayString filenames;
        for (const fs::path& p: pointfiles)
            filenames.Add(static_cast<std::string>(p.filename()));

        // Show dialog with list of pointfiles
        PointFileChooser chooser(filenames);
        if (chooser.ShowModal() != wxID_OK)
        {
            // Dialog cancelled, don't proceed to showing point trace
            return;
        }
    }

    // Hand over to the actual pointfile renderer to toggle visibility
    GlobalCommandSystem().executeCommand("TogglePointfile");
}

}