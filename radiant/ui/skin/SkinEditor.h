#pragma once

#include "icommandsystem.h"

#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dialog/DialogBase.h"

namespace ui
{

class SkinEditor final :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

private:
    SkinEditor();
    ~SkinEditor();

public:
    int ShowModal() override;

    static void ShowDialog(const cmd::ArgumentList& args);
};

}
