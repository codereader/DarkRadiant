#pragma once

#include "ui/materials/MaterialTreeView.h"

namespace ui
{

class MediaBrowserTreeView :
    public MaterialTreeView
{
public:
    MediaBrowserTreeView(wxWindow* parent);

protected:
    void PopulateContextMenu(wxutil::PopupMenu& popupMenu) override;

private:
    bool _testSingleTexSel();
    bool _testLoadInTexView();
    void _onApplyToSel();
    void _onLoadInTexView();
    void _onSelectItems(bool select);
    void _onTreeViewItemActivated(wxDataViewEvent& ev);
};

}
