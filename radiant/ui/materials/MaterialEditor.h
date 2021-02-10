#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"

namespace ui
{

class MaterialEditor :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

private:
    MaterialEditor();

    int ShowModal() override;

public:
    static void ShowDialog(const cmd::ArgumentList& args);
};

}
