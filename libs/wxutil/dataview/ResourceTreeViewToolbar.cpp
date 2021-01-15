#include "ResourceTreeViewToolbar.h"

#include "i18n.h"
#include "iuimanager.h"

#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>

namespace wxutil
{

ResourceTreeViewToolbar::ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView) :
    wxPanel(parent, wxID_ANY),
    _treeView(nullptr),
    _filterEntry(nullptr),
    _showAll(nullptr),
    _showFavourites(nullptr)
{
    auto* grid = new wxFlexGridSizer(2);
    grid->AddGrowableCol(1);

    SetSizer(grid);

    // Hbox for the favourites selection widgets
    auto* favourites = new wxBoxSizer(wxHORIZONTAL);
    _showAll = new wxRadioButton(this, wxID_ANY, _("Show All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    _showFavourites = new wxRadioButton(this, wxID_ANY, _("Show Favourites"));

    _showAll->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);
    _showFavourites->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);

    favourites->Add(_showAll, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 0);
    favourites->Add(_showFavourites, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    // Filter text entry box
    auto* filterBox = new wxBoxSizer(wxHORIZONTAL);

    auto* filterImage = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR, wxSize(16, 16)));

    _filterEntry = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    _filterEntry->SetMinSize(wxSize(100, -1));
    _filterEntry->Bind(wxEVT_TEXT, &ResourceTreeViewToolbar::_onEntryText, this);
    _filterEntry->Bind(wxEVT_CHAR, &ResourceTreeViewToolbar::_onEntryChar, this);
    _filterEntry->SetToolTip(_("Enter search text to filter the tree,\nuse arrow keys to navigate"));

    auto nextImg = wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_down.png");
    _findNextButton = new wxBitmapButton(this, wxID_ANY, nextImg);

    auto prevImg = wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "arrow_up.png");
    _findPrevButton = new wxBitmapButton(this, wxID_ANY, prevImg);

    _findNextButton->SetSize(wxSize(16, 16));
    _findPrevButton->SetSize(wxSize(16, 16));

    _findNextButton->SetToolTip(_("Go to next match"));
    _findPrevButton->SetToolTip(_("Go to previous match"));

    _findNextButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev)
    {
        JumpToNextFilterMatch();
    });
    _findPrevButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev)
    {
        JumpToPrevFilterMatch();
    });

    filterBox->Add(filterImage, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    filterBox->Add(_filterEntry, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    filterBox->Add(_findPrevButton, 0, wxEXPAND | wxRIGHT, 3);
    filterBox->Add(_findNextButton, 0, wxEXPAND, 6);

    grid->Add(favourites, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxRIGHT, 6);
    grid->Add(filterBox, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 6);

    AssociateToTreeView(treeView);
}

void ResourceTreeViewToolbar::AssociateToTreeView(ResourceTreeView* treeView)
{
    _treeView = treeView;
    UpdateFromTreeView();
}

void ResourceTreeViewToolbar::ClearFilter()
{
    _filterEntry->SetValue("");

    if (_treeView != nullptr)
    {
        _treeView->SetFilterText("");
    }
}

void ResourceTreeViewToolbar::JumpToNextFilterMatch()
{
    if (_treeView != nullptr)
    {
        _treeView->JumpToNextFilterMatch();
    }
}

void ResourceTreeViewToolbar::JumpToPrevFilterMatch()
{
    if (_treeView != nullptr)
    {
        _treeView->JumpToPrevFilterMatch();
    }
}

void ResourceTreeViewToolbar::_onEntryChar(wxKeyEvent& ev)
{
    if (ev.GetKeyCode() == WXK_RETURN)
    {
        _treeView->SetFocus();
    }
    else if (ev.GetKeyCode() == WXK_HOME)
    {
        _treeView->JumpToFirstFilterMatch();
    }
    else if (ev.GetKeyCode() == WXK_UP)
    {
        JumpToPrevFilterMatch();
    }
    else if (ev.GetKeyCode() == WXK_DOWN)
    {
        JumpToNextFilterMatch();
    }
    else
    {
        ev.Skip();
    }
}

void ResourceTreeViewToolbar::_onEntryText(wxCommandEvent& ev)
{
    HandleFilterEntryChanged();
}

void ResourceTreeViewToolbar::HandleFilterEntryChanged()
{
    if (_treeView == nullptr)
    {
        return;
    }

    auto filterText = _filterEntry->GetValue();
    bool filterResult = _treeView->SetFilterText(filterText);

    if (!filterText.empty() && !filterResult)
    {
        // No match, set the text to red for user feedback
        _filterEntry->SetForegroundColour(wxColor(220, 0, 0));
    }
    else
    {
        _filterEntry->SetForegroundColour(wxNullColour);
    }

    _filterEntry->Refresh();
}

void ResourceTreeViewToolbar::_onFilterButtonToggled(wxCommandEvent& ev)
{
    if (_treeView == nullptr) return;

    _treeView->SetTreeMode(_showAll->GetValue() ?
        ResourceTreeView::TreeMode::ShowAll :
        ResourceTreeView::TreeMode::ShowFavourites);

    _filterEntry->Clear();
    HandleFilterEntryChanged();
}

void ResourceTreeViewToolbar::UpdateFromTreeView()
{
    if (_treeView == nullptr) return;

    auto mode = _treeView->GetTreeMode();
    _showAll->SetValue(mode == ResourceTreeView::TreeMode::ShowAll);
    _showFavourites->SetValue(mode == ResourceTreeView::TreeMode::ShowFavourites);
}

}
