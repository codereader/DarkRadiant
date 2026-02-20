#pragma once

#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/bmpbuttn.h>
#include <wx/timer.h>
#include <sigc++/signal.h>

#include "ResourceTreeView.h"

namespace wxutil
{

/**
 * Toolbar providing some useful controls for the associated ResourceTreeView
 */
class ResourceTreeViewToolbar :
    public wxPanel
{
private:
    ResourceTreeView* _treeView;

    wxTextCtrl* _filterEntry;

    wxRadioButton* _showAll;
    wxRadioButton* _showFavourites;

    wxBitmapButton* _findPrevButton;
    wxBitmapButton* _findNextButton;

    wxSizer* _leftSizer;
    wxSizer* _rightSizer;

    wxTimer _applyFilterTimer;

    sigc::signal<void(const std::string&)> _signalFilterTextChanged;

public:
    ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView = nullptr);

    // Return the sizer for packing items in the left half of the toolbar
    wxSizer* GetLeftSizer();

    // Return the sizer for packing items in the right half of the toolbar
    wxSizer* GetRightSizer();

    void EnableFavouriteManagement(bool enable);

    void AssociateToTreeView(ResourceTreeView* treeView);

    void ClearFilter();

    // Returns the current filter text
    std::string GetFilterText() const;

    // Signal emitted when the filter text changes
    sigc::signal<void(const std::string&)>& signal_filterTextChanged();

private:
    void JumpToNextFilterMatch();
    void JumpToPrevFilterMatch();

    void _onEntryChar(wxKeyEvent& ev);
    void _onEntryKey(wxKeyEvent& ev);
    void _onEntryText(wxCommandEvent& ev);
    void _onFilterButtonToggled(wxCommandEvent& ev);
    void _onTreeViewFilterTextCleared(wxCommandEvent& ev);
    void _onFilterTimeoutReached(wxTimerEvent& ev);

    void HandleFilterEntryChanged();
    void UpdateFromTreeView();
};

}
