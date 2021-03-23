#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/ResourceTreeView.h"

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

public:
    ImageFileSelector(wxWindow* parent);

    int ShowModal() override;
};

}
