#pragma once

#include "i18n.h"
#include "iuimanager.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/artprov.h>
#include <wx/statbmp.h>
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

    wxBitmapButton* _findPrevButton;
    wxBitmapButton* _findNextButton;

public:
    ResourceTreeViewToolbar(wxWindow* parent, ResourceTreeView* treeView = nullptr) :
        wxPanel(parent, wxID_ANY),
        _treeView(nullptr),
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

        auto* filterEntry = new wxTextCtrl(this, wxID_ANY);
        filterEntry->SetMinSize(wxSize(100, -1));
        filterEntry->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev)
        {
            if (_treeView != nullptr)
            {
                _treeView->SetFilterText(ev.GetString().ToStdString());
            }
        });
        filterEntry->Bind(wxEVT_CHAR, &ResourceTreeViewToolbar::_onEntryChar, this);

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
            jumpToNextFilterMatch();
        });
        _findPrevButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev)
        {
            jumpToPrevFilterMatch();
        });

        filterBox->Add(filterImage, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        filterBox->Add(filterEntry, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        filterBox->Add(_findPrevButton, 0, wxEXPAND | wxRIGHT, 3);
        filterBox->Add(_findNextButton, 0, wxEXPAND, 6);

        grid->Add(favourites, 0, wxALIGN_CENTER_VERTICAL| wxALIGN_LEFT | wxRIGHT, 6);
        grid->Add(filterBox, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 6);

        AssociateToTreeView(treeView);
    }

    void AssociateToTreeView(ResourceTreeView* treeView)
    {
        _treeView = treeView;
        UpdateFromTreeView();
    }

private:
    void jumpToNextFilterMatch()
    {
        if (_treeView != nullptr)
        {
            _treeView->JumpToNextFilterMatch();
        }
    }

    void jumpToPrevFilterMatch()
    {
        if (_treeView != nullptr)
        {
            _treeView->JumpToPrevFilterMatch();
        }
    }

    void _onEntryChar(wxKeyEvent& ev)
    {
        if (ev.GetKeyCode() == WXK_UP)
        {
            jumpToPrevFilterMatch();
        }
        else if (ev.GetKeyCode() == WXK_DOWN)
        {
            jumpToNextFilterMatch();
        }
        else
        {
            ev.Skip();
        }
    }

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
