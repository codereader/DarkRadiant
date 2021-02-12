#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/preview/ModelPreview.h"

#include "ui/common/MaterialTreeView.h"

namespace ui
{

class MaterialEditor :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    MaterialTreeView* _treeView;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

    std::shared_ptr<wxutil::ModelPreview> _preview;

private:
    MaterialEditor();

    int ShowModal() override;

public:
    static void ShowDialog(const cmd::ArgumentList& args);

    void _onClose(wxCommandEvent& ev);
};

}
