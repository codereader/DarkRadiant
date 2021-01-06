#pragma once

#include "i18n.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
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

    wxRadioButton* _showAll;
    wxRadioButton* _showFavourites;

public:
    ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView = nullptr) :
        wxPanel(parent, wxID_ANY),
        _treeView(nullptr),
        _showAll(nullptr),
        _showFavourites(nullptr)
    {
        // Hbox for the favourites selection widgets
        SetSizer(new wxBoxSizer(wxHORIZONTAL));

        _showAll = new wxRadioButton(this, wxID_ANY, _("Show All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        _showFavourites = new wxRadioButton(this, wxID_ANY, _("Show Favourites"));

        _showAll->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);
        _showFavourites->Bind(wxEVT_RADIOBUTTON, &ResourceTreeViewToolbar::_onFilterButtonToggled, this);

        GetSizer()->Add(_showAll, 0, wxRIGHT, 0);
        GetSizer()->Add(_showFavourites, 0, wxLEFT, 6);

        // Filter text entry box
        auto* filterBox = new wxTextCtrl(this, wxID_ANY);
        filterBox->SetMinSize(wxSize(60, -1));
        filterBox->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev)
        {
            if (_treeView != nullptr)
            {
                _treeView->SetFilterText(ev.GetString().ToStdString());
            }
        });
        GetSizer()->Add(filterBox, 1, wxLEFT, 6);

        AssociateToTreeView(treeView);
    }

    void AssociateToTreeView(ResourceTreeView* treeView)
    {
        _treeView = treeView;
        UpdateFromTreeView();
    }

private:
    void _onFilterButtonToggled(wxCommandEvent& ev)
    {
        if (_treeView == nullptr) return;

        _treeView->SetTreeMode(_showAll->GetValue() ?
            ResourceTreeView::TreeMode::ShowAll :
            ResourceTreeView::TreeMode::ShowFavourites);
    }
    
    void UpdateFromTreeView()
    {
        if (_treeView == nullptr) return;

        auto mode = _treeView->GetTreeMode();
        _showAll->SetValue(mode == ResourceTreeView::TreeMode::ShowAll);
        _showFavourites->SetValue(mode == ResourceTreeView::TreeMode::ShowFavourites);
    }
};

}
