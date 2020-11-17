#include "MapSelector.h"

#include "i18n.h"
#include "ifilesystem.h"

#include "string/case_conv.h"
#include <wx/sizer.h>
#include <wx/button.h>

namespace ui
{

// CONSTANTS
namespace
{
    const char* const MAPSELECTOR_TITLE = N_("Choose Map File");
}

MapSelector::MapSelector() :
    DialogBase(_(MAPSELECTOR_TITLE)),
    _treeView(nullptr),
    _handlingSelectionChange(false)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    setupTreeView(this);
    vbox->Add(_treeView, 1, wxEXPAND);

    wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    wxButton* reloadButton = new wxButton(this, wxID_ANY, _("Rescan"));
    reloadButton->Bind(wxEVT_BUTTON, &MapSelector::onRescanPath, this);

    buttonSizer->Prepend(reloadButton, 0, wxRIGHT, 32);

    vbox->Add(buttonSizer, 0, wxALIGN_RIGHT | wxTOP, 12);

    // Set the default size of the window
    _position.connect(this);
    _position.readPosition();

    FitToScreen(0.5f, 0.6f);
}

int MapSelector::ShowModal()
{
    // Populate the tree
    populateTree();

    // Enter the main loop
    return DialogBase::ShowModal();
}

std::string MapSelector::ChooseMapFile()
{
    auto* dialog = new MapSelector();
    std::string returnValue = "";

    if (dialog->ShowModal() == wxID_OK)
    {
        returnValue = dialog->getSelectedPath();
    }

    // Use the instance to select a model.
    return returnValue;
}

void MapSelector::OpenMapFromProject(const cmd::ArgumentList& args)
{
    auto mapPath = ChooseMapFile();

    if (!mapPath.empty())
    {
        GlobalCommandSystem().executeCommand("OpenMap", mapPath);
    }
}

// Helper function to create the TreeView
void MapSelector::setupTreeView(wxWindow* parent)
{
    _treeView = wxutil::FileSystemView::Create(parent, wxBORDER_STATIC | wxDV_NO_HEADER);

    // Get the extensions from all possible patterns (e.g. *.map or *.mapx)
    FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(filetype::TYPE_MAP);

    std::set<std::string> fileExtensions;

    for (const auto& pattern : patterns)
    {
        fileExtensions.insert(pattern.extension);
    }

    // Add all PK extensions too
    const auto& pakExtensions = GlobalFileSystem().getArchiveExtensions();

    for (const auto& extension : pakExtensions)
    {
        fileExtensions.insert(string::to_lower_copy(extension));
    }

    _treeView->SetFileExtensions(fileExtensions);
}

std::string MapSelector::getBaseFolder()
{
    return ""; // use an empty path which resembles the VFS root
}

void MapSelector::populateTree()
{
    _treeView->SetBasePath(getBaseFolder());
    _treeView->Populate();
}

void MapSelector::onRescanPath(wxCommandEvent& ev)
{
    populateTree();
}

std::string MapSelector::getSelectedPath()
{
    return _treeView->GetSelectedPath();
}

}
