#pragma once

#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/bmpbuttn.h>

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

public:
    ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView = nullptr);

    void AssociateToTreeView(ResourceTreeView* treeView);

    void ClearFilter();

private:
    void JumpToNextFilterMatch();
    void JumpToPrevFilterMatch();

    void _onEntryChar(wxKeyEvent& ev);
    void _onEntryText(wxCommandEvent& ev);
    void _onFilterButtonToggled(wxCommandEvent& ev);
    
    void UpdateFromTreeView();
};

}
