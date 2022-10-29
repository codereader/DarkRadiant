#include "ResourceTreeViewToolbar.h"

#include "i18n.h"

#include "wxutil/Bitmap.h"
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>

namespace wxutil
{

namespace
{
    constexpr int APPLY_FILTER_TEXT_DELAY_MSEC = 250;
}

ResourceTreeViewToolbar::ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView) :
    wxPanel(parent, wxID_ANY),
    _treeView(nullptr),
    _filterEntry(nullptr),
    _showAll(nullptr),
    _showFavourites(nullptr),
    _applyFilterTimer(this)
{
    auto* grid = new wxFlexGridSizer(2);
    grid->AddGrowableCol(1);

    SetSizer(grid);

    // Hbox for the favourites selection widgets
    _leftSizer = new wxBoxSizer(wxHORIZONTAL);
    _showAll = new wxRadioButton(this, wxID_ANY, _("Show All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    _showFavourites = new wxRadioButton(this, wxID_ANY, _("Show Favourites"));

    _showAll->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);
    _showFavourites->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);

    _leftSizer->Add(_showAll, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 0);
    _leftSizer->Add(_showFavourites, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    // Filter text entry box
    _rightSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* filterImage = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR, wxSize(16, 16)));

    _filterEntry = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    _filterEntry->SetMinSize(wxSize(100, -1));
    _filterEntry->Bind(wxEVT_TEXT, &ResourceTreeViewToolbar::_onEntryText, this);
    _filterEntry->Bind(wxEVT_CHAR, &ResourceTreeViewToolbar::_onEntryChar, this);
    _filterEntry->Bind(wxEVT_CHAR_HOOK, &ResourceTreeViewToolbar::_onEntryKey, this);
    _filterEntry->SetToolTip(_("Enter search text to filter the tree,\nuse arrow keys to navigate"));

    auto nextImg = wxutil::GetLocalBitmap("arrow_down.png");
    _findNextButton = new wxBitmapButton(this, wxID_ANY, nextImg);

    auto prevImg = wxutil::GetLocalBitmap("arrow_up.png");
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

    _rightSizer->Add(filterImage, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    _rightSizer->Add(_filterEntry, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    _rightSizer->Add(_findPrevButton, 0, wxEXPAND | wxRIGHT, 3);
    _rightSizer->Add(_findNextButton, 0, wxEXPAND, 6);

    grid->Add(_leftSizer, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxRIGHT, 6);
    grid->Add(_rightSizer, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 6);

    AssociateToTreeView(treeView);

    Bind(wxEVT_TIMER, &ResourceTreeViewToolbar::_onFilterTimeoutReached, this);
}

wxSizer* ResourceTreeViewToolbar::GetLeftSizer()
{
    return _leftSizer;
}

wxSizer* ResourceTreeViewToolbar::GetRightSizer()
{
    return _rightSizer;
}

void ResourceTreeViewToolbar::EnableFavouriteManagement(bool enable)
{
    _showAll->Show(enable);
    _showFavourites->Show(enable);
}

void ResourceTreeViewToolbar::AssociateToTreeView(ResourceTreeView* treeView)
{
    _treeView = treeView;

    if (_treeView != nullptr)
    {
        _treeView->Bind(EV_TREEVIEW_FILTERTEXT_CLEARED, &ResourceTreeViewToolbar::_onTreeViewFilterTextCleared, this);
    }

    UpdateFromTreeView();
}

void ResourceTreeViewToolbar::ClearFilter()
{
    _applyFilterTimer.Stop();
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

void ResourceTreeViewToolbar::_onEntryKey(wxKeyEvent& ev)
{
    if (ev.GetKeyCode() == WXK_ESCAPE && !_filterEntry->GetValue().empty())
    {
        // Clear the search box on esc and focus the tree view
        ClearFilter();
        _treeView->SetFocus();
        return;
    }

    ev.Skip();
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

    // Put in a slight delay before actually firing off the search (#5745)
    _applyFilterTimer.Start(APPLY_FILTER_TEXT_DELAY_MSEC, true);
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

void ResourceTreeViewToolbar::_onTreeViewFilterTextCleared(wxCommandEvent& ev)
{
    // Tree view cleared the filter, clear our entry box
    _filterEntry->Clear();
    _applyFilterTimer.Stop();
    ev.Skip();
}

void ResourceTreeViewToolbar::UpdateFromTreeView()
{
    if (_treeView == nullptr) return;

    auto mode = _treeView->GetTreeMode();
    _showAll->SetValue(mode == ResourceTreeView::TreeMode::ShowAll);
    _showFavourites->SetValue(mode == ResourceTreeView::TreeMode::ShowFavourites);
}

void ResourceTreeViewToolbar::_onFilterTimeoutReached(wxTimerEvent& ev)
{
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

}
