#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include <wx/button.h>
#include <wx/textctrl.h>

namespace ui
{

/**
 * Modal dialog to select an image file (TGA, DDS, etc.) from the VFS
 */
class ImageFileSelector :
    public wxutil::DialogBase
{
private:
    wxutil::ResourceTreeView::Columns _columns;
    wxutil::ResourceTreeView* _treeView;

    wxButton* _okButton;
    wxTextCtrl* _targetControl;
    std::string _previousValue;

public:
    ImageFileSelector(wxWindow* parent, wxTextCtrl* targetControl);

    int ShowModal() override;

    std::string GetSelection();

private:
    void onTreeSelectionChanged(wxDataViewEvent& ev);
};

}
