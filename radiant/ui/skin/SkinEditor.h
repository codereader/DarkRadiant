#pragma once

#include "icommandsystem.h"

#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dialog/DialogBase.h"

namespace ui
{

class ModelTreeView;

class SkinEditor final :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    ModelTreeView* _modelTreeView;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

private:
    SkinEditor();
    ~SkinEditor();

public:
    int ShowModal() override;

    static void ShowDialog(const cmd::ArgumentList& args);

private:
    void setupModelTreeView();

    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
