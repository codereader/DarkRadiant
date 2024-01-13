#include "MapSelector.h"

#include "i18n.h"
#include "ifiletypes.h"
#include "ifilesystem.h"

#include "string/case_conv.h"
#include "os/path.h"
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
    _dialogPanel(nullptr),
    _openButton(nullptr),
    _reloadButton(nullptr),
    _treeView(nullptr),
    _useModPath(nullptr),
    _useCustomPath(nullptr),
    _customPath(nullptr),
    _handlingSelectionChange(false)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

    setupPathSelector(vbox);

    setupTreeView(this);
    vbox->Add(_treeView, 1, wxEXPAND);

    auto* cancelButton = new wxButton(this, wxID_CANCEL, _("Cancel"));
    _openButton = new wxButton(this, wxID_ANY, _("Open"));
    _reloadButton = new wxButton(this, wxID_ANY, _("Refresh"));

    _openButton->Bind(wxEVT_BUTTON, &MapSelector::onOpenPath, this);
    _reloadButton->Bind(wxEVT_BUTTON, &MapSelector::onRescanPath, this);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonSizer->Add(_reloadButton, 0, wxRIGHT, 24);
    buttonSizer->Add(_openButton, 0, wxRIGHT, 6);
    buttonSizer->Add(cancelButton, 0, wxRIGHT, 0);

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
    updateButtons();

    // Enter the main loop
    return DialogBase::ShowModal();
}

MapSelector::Result MapSelector::ChooseMapFile()
{
    auto* dialog = new MapSelector();
    Result returnValue;

    if (dialog->ShowModal() == wxID_OK)
    {
        returnValue.selectedPath = dialog->getSelectedPath();
        returnValue.archivePath = dialog->getArchivePath();
    }

    // Use the instance to select a model.
    return returnValue;
}

void MapSelector::OpenMapFromProject(const cmd::ArgumentList& args)
{
    auto result = ChooseMapFile();

    if (result.selectedPath.empty())
    {
        return;
    }

    auto extension = string::to_lower_copy(os::getExtension(result.archivePath));

    if (!os::isDirectory(result.archivePath) && GlobalFileSystem().getArchiveExtensions().count(extension) > 0)
    {
        GlobalCommandSystem().executeCommand("OpenMapFromArchive", result.archivePath, result.selectedPath);
    }
    else
    {
        GlobalCommandSystem().executeCommand("OpenMap", result.selectedPath);
    }
}

// Helper function to create the TreeView
void MapSelector::setupTreeView(wxWindow* parent)
{
    _treeView = wxutil::FileSystemView::Create(parent, wxBORDER_STATIC | wxDV_NO_HEADER);
    _treeView->Bind(wxutil::EV_FSVIEW_SELECTION_CHANGED, &MapSelector::onSelectionChanged, this);
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &MapSelector::onItemActivated, this);;
    _treeView->signal_TreePopulated().connect(sigc::mem_fun(*this, &MapSelector::onFileViewTreePopulated));

    // Get the extensions from all possible patterns (e.g. *.map or *.mapx)
    FileTypePatterns patterns = GlobalFiletypes().getPatternsForType(filetype::TYPE_MAP);

    for (const auto& pattern : patterns)
    {
        _mapFileExtensions.insert(pattern.extension);
    }

    std::set<std::string> fileExtensions = _mapFileExtensions;

    // Add all PK extensions too
    const auto& pakExtensions = GlobalFileSystem().getArchiveExtensions();

    for (const auto& extension : pakExtensions)
    {
        fileExtensions.insert(string::to_lower_copy(extension));
    }

    _treeView->SetFileExtensions(fileExtensions);
}

void MapSelector::setupPathSelector(wxSizer* parentSizer)
{
    // Path selection box
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    _useModPath = new wxRadioButton(this, wxID_ANY, _("Browse mod resources"),
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

    _useCustomPath = new wxRadioButton(this, wxID_ANY, _("Browse custom PAK:"));
    _customPath = new wxutil::PathEntry(this, filetype::TYPE_PAK, true);

    // Connect to the changed event
    _customPath->Bind(wxutil::EV_PATH_ENTRY_CHANGED, [&](wxCommandEvent& ev)
    {
        if (!_customPath->getValue().empty())
        {
            _useCustomPath->SetValue(true);
        }
        else
        {
            _useModPath->SetValue(true);
        }
        onPathSelectionChanged();
    });

    hbox->Add(_useModPath, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    hbox->Add(_useCustomPath, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
    hbox->Add(_customPath, 1, wxLEFT, 6);

    parentSizer->Add(hbox, 0, wxBOTTOM | wxEXPAND, 12);

    // Wire up the signals
    _useModPath->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& ev)
    {
        onPathSelectionChanged();
    });

    _useCustomPath->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent& ev)
    {
        onPathSelectionChanged();
    });
}

void MapSelector::onPathSelectionChanged()
{
    // Re-populate the tree if the base path is different
    auto basePath = getBasePath();

    if (_treeView->GetBasePath() != basePath)
    {
        populateTree();
    }
}

std::string MapSelector::getBasePath()
{
    if (_useCustomPath->GetValue() && !_customPath->getValue().empty())
    {
        return _customPath->getValue();
    }

    return ""; // use an empty path which resembles the VFS root
}

void MapSelector::populateTree()
{
    _treeView->SetBasePath(getBasePath());
    _treeView->Populate();
}

void MapSelector::onOpenPath(wxCommandEvent& ev)
{
    handleItemActivated();
}

void MapSelector::onRescanPath(wxCommandEvent& ev)
{
    _reloadButton->Disable();
    populateTree();
}

void MapSelector::updateButtons()
{
    if (!_treeView->GetSelection().IsOk() || _treeView->GetIsFolderSelected())
    {
        _openButton->Disable();
        return;
    }

    _openButton->Enable();
}

void MapSelector::onSelectionChanged(wxutil::FileSystemView::SelectionChangedEvent& ev)
{
    updateButtons();
}

void MapSelector::handleItemActivated()
{
    auto selectedPath = _treeView->GetSelectedPath();
    auto extension = string::to_lower_copy(os::getExtension(selectedPath));

    if (GlobalFileSystem().getArchiveExtensions().count(extension) > 0)
    {
        // Check if this is a physical file
        auto rootPath = GlobalFileSystem().findFile(selectedPath);

        auto pathToPak = !rootPath.empty() ?
            os::standardPathWithSlash(rootPath) + selectedPath :
            selectedPath;

        _useCustomPath->SetValue(true);
        _customPath->setValue(pathToPak);
        onPathSelectionChanged();
        return;
    }

    // If this is a file item, double-click will select and close the dialog
    if (_treeView->GetIsFolderSelected())
    {
        return;
    }

    EndModal(wxID_OK);
}

void MapSelector::onItemActivated(wxDataViewEvent& ev)
{
    handleItemActivated();
}

void MapSelector::onFileViewTreePopulated()
{
    _treeView->ExpandPath("maps/");
    _reloadButton->Enable();
}

std::string MapSelector::getSelectedPath()
{
    return _treeView->GetSelectedPath();
}

std::string MapSelector::getArchivePath()
{
    return _treeView->GetArchivePathOfSelection();
}

}
