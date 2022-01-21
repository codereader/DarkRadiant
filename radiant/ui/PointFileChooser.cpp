#include "PointFileChooser.h"

#include "ui/imainframe.h"
#include "imap.h"
#include "i18n.h"
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
           _("Choose pointfile"))
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    // Construct and populate the dropdown list
    _pfChoice.reset(
        new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, files)
    );
    _pfChoice->SetSelection(0);
    GetSizer()->Add(_pfChoice.get(), 0, wxEXPAND | wxALL, 12);

    // Add dialog buttons
    auto btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    GetSizer()->Add(btnSizer, 0, wxALIGN_RIGHT | wxBOTTOM, 12);

    Fit();
}

void PointFileChooser::chooseAndToggle()
{
    // If the point trace is currently visible, toggle it to invisible and we're
    // done.
    if (GlobalMapModule().isPointTraceVisible())
    {
        GlobalMapModule().showPointFile({});
        return;
    }

    // Determine the list of pointfiles
    std::vector<fs::path> pointfiles;
    GlobalMapModule().forEachPointfile([&](const fs::path& p)
                                       { pointfiles.push_back(p); });
    if (pointfiles.empty())
        throw cmd::ExecutionFailure("No pointfiles found for current map.");

    // If there is a choice to make, show the dialog
    std::size_t chosenPointfile = 0;
    if (pointfiles.size() > 1)
    {
        // Construct list of wxString filenames
        wxArrayString filenames;
        for (const fs::path& p: pointfiles)
            filenames.Add(p.filename().string());

        // Show dialog with list of pointfiles
        PointFileChooser chooser(filenames);
        if (chooser.ShowModal() == wxID_OK)
            chosenPointfile = chooser._pfChoice->GetSelection();
        else
            // Dialog cancelled, don't proceed to showing point trace
            return;
    }

    // Show the chosen (or only) pointfile
    if (chosenPointfile >= 0 && chosenPointfile < pointfiles.size())
        GlobalMapModule().showPointFile(pointfiles[chosenPointfile]);
}

}